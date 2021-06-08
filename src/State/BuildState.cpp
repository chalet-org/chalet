/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildState.hpp"

#include "BuildJson/BuildJsonParser.hpp"
#include "CacheJson/CacheJsonParser.hpp"
#include "Dependencies/DependencyManager.hpp"
#include "Libraries/Format.hpp"
#include "State/Target/ProjectTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
BuildState::BuildState(const CommandLineInputs& inInputs) :
	m_inputs(inInputs),
	info(m_inputs),
	compilerTools(m_inputs, *this),
	paths(m_inputs, info),
	environment(paths),
	msvcEnvironment(*this),
	cache(info, paths),
	sourceCache(cache)
{
}

/*****************************************************************************/
bool BuildState::initialize(const bool inInstallDependencies)
{
	if (!parseCacheJson())
		return false;

	enforceArchitectureInPath();

	if (!parseBuildJson())
		return false;

	if (inInstallDependencies)
	{
		if (!installDependencies())
			return false;
	}

	if (!initializeBuild())
		return false;

	if (!validateState())
		return false;

	return true;
}

/*****************************************************************************/
bool BuildState::parseCacheJson()
{
	CacheJsonParser parser(m_inputs, *this);
	return parser.serialize();
}

/*****************************************************************************/
bool BuildState::parseBuildJson()
{
	BuildJsonParser parser(m_inputs, *this, m_inputs.buildFile());
	return parser.serialize();
}

/*****************************************************************************/
bool BuildState::installDependencies()
{
	const auto& command = m_inputs.command();
	const bool cleanOutput = true;

	DependencyManager depMgr(*this, cleanOutput);
	if (!depMgr.run(command == Route::Configure))
	{
		Diagnostic::error("There was an error creating the dependencies.");
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildState::initializeBuild()
{
	if (!tools.fetchVersions())
		return false;

	Timer timer;

	Diagnostic::info("Initializing State", false);

	// Note: This is about as quick as it'll get (50ms in mingw)
	if (!compilerTools.initialize(targets))
	{
		const auto& targetArch = compilerTools.detectedToolchain() == ToolchainType::GNU ?
			  m_inputs.targetArchitecture() :
			  info.targetArchitectureString();

		Diagnostic::error(fmt::format("Requested arch '{}' is not supported.", targetArch));
		return false;
	}

	paths.initialize();

	// Note: < 1ms
	for (auto& target : targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<ProjectTarget&>(*target);
			auto& compilerConfig = compilerTools.getConfig(project.language());
			project.parseOutputFilename(compilerConfig);

			if (!project.isStaticLibrary())
			{
				std::string intermediateDir = paths.intermediateDir();
				project.addLocation(intermediateDir);
			}

			const bool isMsvc = compilerConfig.isMsvc();
			if (!isMsvc)
			{
				std::string libDir = compilerConfig.compilerPathLib();
				project.addLibDir(libDir);

				std::string includeDir = compilerConfig.compilerPathInclude();
				project.addIncludeDir(includeDir);
			}

			for (auto& t : targets)
			{
				if (t->isProject())
				{
					auto& p = static_cast<ProjectTarget&>(*t);
					bool staticLib = p.kind() == ProjectKind::StaticLibrary;
					project.resolveLinksFromProject(p.name(), staticLib);
				}
			}
		}
	}

	{
		// Note: Most time is spent here (277ms in mingw)
		environment.initialize();

		for (auto& target : targets)
		{
			target->initialize();
		}

		initializeCache();
	}

	Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
void BuildState::initializeCache()
{
	cache.initialize(m_inputs.appPath());

	cache.checkIfCompileStrategyChanged();
	cache.checkIfWorkingDirectoryChanged();

	cache.removeStaleProjectCaches(BuildCache::Type::Local);
	cache.removeBuildIfCacheChanged(paths.buildOutputDir());
	cache.saveEnvironmentCache();

	sourceCache.initialize();
	sourceCache.save();
}

/*****************************************************************************/
bool BuildState::validateState()
{
	{
		auto workingDirectory = Commands::getWorkingDirectory();
		Path::sanitize(workingDirectory, true);

		if (String::toLowerCase(paths.workingDirectory()) != String::toLowerCase(workingDirectory))
		{
			if (!Commands::changeWorkingDirectory(paths.workingDirectory()))
			{
				Diagnostic::error(fmt::format("Error changing directory to '{}'", paths.workingDirectory()));
				return false;
			}
		}
	}

	bool hasCMakeTargets = false;
	bool hasSubChaletTargets = false;
	for (auto& target : targets)
	{
		if (target->isSubChalet())
		{
			hasSubChaletTargets = true;
		}
		else if (target->isCMake())
		{
			hasCMakeTargets = true;
		}

		if (!target->validate())
		{
			Diagnostic::error(fmt::format("Error validating the '{}' target.", target->name()));
			return false;
		}
	}

	if (hasCMakeTargets)
	{
		if (!tools.fetchCmakeVersion())
		{
			Diagnostic::error(fmt::format("The path to the CMake executable could not be resolved: {}", tools.cmake()));
			return false;
		}
	}
	if (hasSubChaletTargets)
	{
		if (!tools.resolveOwnExecutable(m_inputs.appPath()))
		{
			Diagnostic::error(fmt::format("(Welp.) The path to the chalet executable could not be resolved: {}", tools.chalet()));
			return false;
		}
	}

	for (auto& target : distribution)
	{
		if (!target->validate())
		{
			Diagnostic::error(fmt::format("Error validating the '{}' distribution target.", target->name()));
			return false;
		}
	}

	auto strategy = environment.strategy();
	if (strategy == StrategyType::Makefile)
	{
		const auto& makeExec = tools.make();
		if (makeExec.empty() || !Commands::pathExists(makeExec))
		{
			Diagnostic::error(fmt::format("{} was either not defined in the cache, or not found.", makeExec.empty() ? "make" : makeExec));
			return false;
		}
	}
	else if (strategy == StrategyType::Ninja)
	{
		auto& ninjaExec = tools.ninja();
		if (ninjaExec.empty() || !Commands::pathExists(ninjaExec))
		{
			Diagnostic::error(fmt::format("{} was either not defined in the cache, or not found.", ninjaExec.empty() ? "ninja" : ninjaExec));
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
void BuildState::enforceArchitectureInPath()
{
#if defined(CHALET_WIN32)
	Arch::Cpu targetArch = info.targetArchitecture();
	auto path = Environment::getPath();
	if (String::contains({ "/mingw64/", "/mingw32/" }, path))
	{

		std::string lower = String::toLowerCase(path);
		if (targetArch == Arch::Cpu::X64)
		{
			auto start = lower.find("/mingw32/");
			if (start != std::string::npos)
			{
				String::replaceAll(path, path.substr(start, 9), "/mingw64/");
				Environment::setPath(path);
			}
		}
		else if (targetArch == Arch::Cpu::X86)
		{
			auto start = lower.find("/mingw64/");
			if (start != std::string::npos)
			{
				String::replaceAll(path, path.substr(start, 9), "/mingw32/");
				Environment::setPath(path);
			}
		}
	}
	else if (String::contains({ "/clang64/", "/clang32/" }, path))
	{
		// TODO: clangarm64

		std::string lower = String::toLowerCase(path);
		if (targetArch == Arch::Cpu::X64)
		{
			auto start = lower.find("/clang32/");
			if (start != std::string::npos)
			{
				String::replaceAll(path, path.substr(start, 9), "/clang64/");
				Environment::setPath(path);
			}
		}
		else if (targetArch == Arch::Cpu::X86)
		{
			auto start = lower.find("/clang64/");
			if (start != std::string::npos)
			{
				String::replaceAll(path, path.substr(start, 9), "/clang32/");
				Environment::setPath(path);
			}
		}
	}
#else
#endif
}
}
