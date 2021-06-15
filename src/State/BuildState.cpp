/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildState.hpp"

#include "BuildJson/BuildJsonParser.hpp"
#include "Builder/BuildManager.hpp"
#include "CacheJson/CacheToolchainParser.hpp"
#include "Dependencies/DependencyManager.hpp"
#include "Libraries/Format.hpp"
#include "State/AncillaryTools.hpp"
#include "State/StatePrototype.hpp"
#include "State/Target/ProjectTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
BuildState::BuildState(CommandLineInputs inInputs, StatePrototype& inJsonPrototype) :
	m_inputs(std::move(inInputs)),
	m_prototype(inJsonPrototype),
	ancillaryTools(m_prototype.ancillaryTools),
	distribution(m_prototype.distribution),
	environment(m_prototype.environment),
	cache(m_prototype.cache),
	info(m_inputs),
	toolchain(m_inputs, *this),
	paths(m_inputs, info),
	msvcEnvironment(*this),
	sourceCache(m_prototype.cache, info)
{
}

/*****************************************************************************/
bool BuildState::initialize(const bool inInstallDependencies)
{
	if (!parseCacheJson())
		return false;

	enforceArchitectureInPath();

	if (!initializeBuildConfiguration())
		return false;

	if (!parseBuildJson())
		return false;

	if (inInstallDependencies)
	{
		if (!installDependencies())
			return false;
	}

	if (!initializeBuild())
		return false;

	return true;
}

/*****************************************************************************/
bool BuildState::doBuild(const bool inShowSuccess)
{
	BuildManager mgr(m_inputs, *this);
	return mgr.run(m_inputs.command(), inShowSuccess);
}

bool BuildState::doBuild(const Route inRoute, const bool inShowSuccess)
{
	BuildManager mgr(m_inputs, *this);
	return mgr.run(inRoute, inShowSuccess);
}

/*****************************************************************************/
void BuildState::saveCaches()
{
	m_prototype.cache.saveLocalConfig();
	sourceCache.save();
}

/*****************************************************************************/
bool BuildState::initializeBuildConfiguration()
{
	auto& config = m_inputs.buildConfiguration();
	const auto& buildConfigurations = m_prototype.buildConfigurations();

	if (buildConfigurations.find(config) == buildConfigurations.end())
	{
		Diagnostic::error(fmt::format("{}: The build configuration '{}' was not found.", m_inputs.buildFile(), config));
		return false;
	}

	configuration = buildConfigurations.at(config);
	info.setBuildConfiguration(config);

	return true;
}

/*****************************************************************************/
bool BuildState::parseCacheJson()
{
	auto& cacheFile = m_prototype.cache.localConfig();
	CacheToolchainParser parser(m_inputs, *this, cacheFile);
	return parser.serialize();
}

/*****************************************************************************/
bool BuildState::parseBuildJson()
{
	BuildJsonParser parser(m_inputs, m_prototype, *this);
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
	Timer timer;

	Diagnostic::info("Initializing Build", false);

	// Note: This is about as quick as it'll get (50ms in mingw)
	if (!toolchain.initialize(targets))
	{
		const auto& targetArch = m_inputs.toolchainPreference().type == ToolchainType::GNU ?
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
			auto& compilerConfig = toolchain.getConfig(project.language());
			project.parseOutputFilename(compilerConfig);

			if (!project.isStaticLibrary())
			{
				std::string intermediateDir = paths.intermediateDir();
				project.addLocation(std::move(intermediateDir));
			}

			const bool isMsvc = compilerConfig.isMsvc();
			if (!isMsvc)
			{
				std::string libDir = compilerConfig.compilerPathLib();
				project.addLibDir(std::move(libDir));

				std::string includeDir = compilerConfig.compilerPathInclude();
				project.addIncludeDir(std::move(includeDir));
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
		m_prototype.environment.initialize(paths);

		for (auto& target : targets)
		{
			target->initialize();
		}

		initializeCache();
	}

	if (!validateState())
		return false;

	Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
void BuildState::initializeCache()
{
	m_prototype.cache.checkIfCompileStrategyChanged(m_inputs.toolchainPreferenceRaw());
	m_prototype.cache.checkIfWorkingDirectoryChanged();

	m_prototype.cache.removeStaleProjectCaches(m_inputs.toolchainPreferenceRaw(), WorkspaceCache::Type::Local);
	m_prototype.cache.removeBuildIfCacheChanged(paths.buildOutputDir());
	m_prototype.cache.saveLocalConfig();

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

	auto strat = toolchain.strategy();
	if (strat == StrategyType::Makefile)
	{
		toolchain.fetchMakeVersion();

		const auto& makeExec = toolchain.make();
		if (makeExec.empty() || !Commands::pathExists(makeExec))
		{
			Diagnostic::error(fmt::format("{} was either not defined in the cache, or not found.", makeExec.empty() ? "make" : makeExec));
			return false;
		}
	}
	else if (strat == StrategyType::Ninja)
	{
		toolchain.fetchNinjaVersion();

		auto& ninjaExec = toolchain.ninja();
		if (ninjaExec.empty() || !Commands::pathExists(ninjaExec))
		{
			Diagnostic::error(fmt::format("{} was either not defined in the cache, or not found.", ninjaExec.empty() ? "ninja" : ninjaExec));
			return false;
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
	}

	if (hasCMakeTargets)
	{
		if (!toolchain.fetchCmakeVersion())
		{
			Diagnostic::error(fmt::format("The path to the CMake executable could not be resolved: {}", toolchain.cmake()));
			return false;
		}
	}
	if (hasSubChaletTargets)
	{
		if (!m_prototype.ancillaryTools.resolveOwnExecutable(m_inputs.appPath()))
		{
			Diagnostic::error(fmt::format("(Welp.) The path to the chalet executable could not be resolved: {}", m_prototype.ancillaryTools.chalet()));
			return false;
		}
	}

	for (auto& target : targets)
	{
		// must validate after cmake/sub-chalet check
		if (!target->validate())
		{
			Diagnostic::error(fmt::format("Error validating the '{}' target.", target->name()));
			return false;
		}
	}

	if (configuration.enableProfiling())
	{
#if defined(CHALET_MACOS)
		m_prototype.ancillaryTools.fetchXcodeVersion();
#endif
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
