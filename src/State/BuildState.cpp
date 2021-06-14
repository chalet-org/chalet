/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildState.hpp"

#include "BuildJson/BuildJsonParser.hpp"
#include "Builder/BuildManager.hpp"
#include "CacheJson/CacheJsonParser.hpp"
#include "Dependencies/DependencyManager.hpp"
#include "Libraries/Format.hpp"
#include "State/CacheTools.hpp"
#include "State/StatePrototype.hpp"
#include "State/Target/ProjectTarget.hpp"
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
	tools(m_prototype.tools),
	distribution(m_prototype.distribution),
	info(m_inputs),
	compilerTools(m_inputs, *this),
	paths(m_inputs, info),
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

	if (!cache.createCacheFolder(BuildCache::Type::Local))
	{
		Diagnostic::error("There was an error creating the build cache.");
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildState::doBuild()
{
	BuildManager mgr(m_inputs, *this);
	return mgr.run(m_inputs.command());
}

bool BuildState::doBuild(const Route inRoute)
{
	BuildManager mgr(m_inputs, *this);
	return mgr.run(inRoute);
}

/*****************************************************************************/
void BuildState::saveCaches()
{
	cache.saveLocalConfig();
	sourceCache.save();
}

/*****************************************************************************/
const StringList& BuildState::environmentPath() const noexcept
{
	return m_prototype.environment.path();
}

void BuildState::addEnvironmentPaths(StringList&& inList)
{
	m_prototype.environment.addPaths(std::move(inList));
}

/*****************************************************************************/
bool BuildState::dumpAssembly() const noexcept
{
	return m_prototype.environment.dumpAssembly();
}

/*****************************************************************************/
bool BuildState::showCommands() const noexcept
{
	return m_prototype.environment.showCommands();
}

/*****************************************************************************/
uint BuildState::maxJobs() const noexcept
{
	return m_prototype.environment.maxJobs();
}

/*****************************************************************************/
StrategyType BuildState::strategy() const noexcept
{
	return m_prototype.environment.strategy();
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
	auto& cacheFile = cache.localConfig();
	CacheJsonParser parser(m_inputs, m_prototype, *this, cacheFile);
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
	if (!m_prototype.tools.fetchVersions())
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
	cache.initialize(m_inputs.appPath());

	cache.checkIfCompileStrategyChanged();
	cache.checkIfWorkingDirectoryChanged();

	cache.removeStaleProjectCaches(BuildCache::Type::Local);
	cache.removeBuildIfCacheChanged(paths.buildOutputDir());
	cache.saveLocalConfig();

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
		if (!m_prototype.tools.fetchCmakeVersion())
		{
			Diagnostic::error(fmt::format("The path to the CMake executable could not be resolved: {}", m_prototype.tools.cmake()));
			return false;
		}
	}
	if (hasSubChaletTargets)
	{
		if (!m_prototype.tools.resolveOwnExecutable(m_inputs.appPath()))
		{
			Diagnostic::error(fmt::format("(Welp.) The path to the chalet executable could not be resolved: {}", m_prototype.tools.chalet()));
			return false;
		}
	}

	if (configuration.enableProfiling())
	{
#if defined(CHALET_MACOS)
		m_prototype.tools.fetchXcodeVersion();
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
