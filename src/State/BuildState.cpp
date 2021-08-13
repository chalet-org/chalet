/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildState.hpp"

#include "BuildJson/BuildJsonParser.hpp"
#include "Builder/BuildManager.hpp"
#include "SettingsJson/SettingsToolchainJsonParser.hpp"

#include "State/AncillaryTools.hpp"
#include "State/StatePrototype.hpp"
#include "State/Target/ProjectTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
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
	cache(m_prototype.cache),
	externalDependencies(m_prototype.externalDependencies),
	info(m_inputs),
	environment(m_prototype.environment), // copy
	toolchain(m_inputs, *this),
	paths(m_inputs, inJsonPrototype.environment),
	msvcEnvironment(m_inputs, *this)
{
}

/*****************************************************************************/
bool BuildState::initialize()
{
	enforceArchitectureInPath();

	if (!parseToolchainFromSettingsJson())
		return false;

	if (!initializeBuildConfiguration())
		return false;

	if (!parseBuildJson())
		return false;

	if (!initializeBuild())
		return false;

	manageLibraryPathVariables();

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
std::string BuildState::getUniqueIdForState(const StringList& inOther) const
{
	std::string ret;
	const auto& hostArch = info.hostArchitectureString();
	const auto targetArch = m_inputs.getArchWithOptionsAsString(info.targetArchitectureString());
	const auto& toolchainPref = m_inputs.toolchainPreferenceName();
	const auto& strategy = toolchain.strategyString();
	const auto& buildConfig = info.buildConfiguration();

	std::string showCmds;
	if (toolchain.strategy() != StrategyType::Ninja)
	{
		showCmds = std::to_string(Output::showCommands() ? 1 : 0);
	}

	ret = fmt::format("{}_{}_{}_{}_{}_{}_{}", hostArch, targetArch, toolchainPref, strategy, buildConfig, showCmds, String::join(inOther, '_'));

	return Hash::string(ret);
}

/*****************************************************************************/
bool BuildState::initializeBuildConfiguration()
{
	auto config = m_inputs.buildConfiguration();
	if (config.empty())
	{
		config = m_prototype.anyConfiguration();
	}

	const auto& buildConfigurations = m_prototype.buildConfigurations();

	if (buildConfigurations.find(config) == buildConfigurations.end())
	{
		Diagnostic::error("{}: The build configuration '{}' was not found.", m_inputs.inputFile(), config);
		return false;
	}

	configuration = buildConfigurations.at(config);
	info.setBuildConfiguration(config);

	return true;
}

/*****************************************************************************/
bool BuildState::parseToolchainFromSettingsJson()
{
	auto& cacheFile = m_prototype.cache.getSettings(SettingsType::Local);
	SettingsToolchainJsonParser parser(m_inputs, *this, cacheFile);
	return parser.serialize();
}

/*****************************************************************************/
bool BuildState::parseBuildJson()
{
	BuildJsonParser parser(m_inputs, m_prototype, *this);
	return parser.serialize();
}

/*****************************************************************************/
bool BuildState::initializeBuild()
{
	Timer timer;

	Output::setShowCommandOverride(false);

	bool isConfigure = m_inputs.command() == Route::Configure;

	if (!isConfigure)
		Diagnostic::infoEllipsis("Initializing");

	{
		auto& settingsFile = m_prototype.cache.getSettings(SettingsType::Local);
		// Note: This is about as quick as it'll get (50ms in mingw)
		if (!toolchain.initialize(targets, settingsFile))
		{
			const auto& targetArch = m_inputs.toolchainPreference().type == ToolchainType::GNU ?
				  m_inputs.targetArchitecture() :
				  info.targetArchitectureString();

			auto& toolchainName = m_inputs.toolchainPreferenceName();
			Diagnostic::error("Requested arch '{}' is not supported by the '{}' toolchain.", targetArch, toolchainName);
			return false;
		}
	}

	paths.initialize(info, toolchain);

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
				if (project.files().size() > 0)
				{
					StringList locations;
					for (auto& file : project.files())
					{
						auto loc = String::getPathFolder(file);
						List::addIfDoesNotExist(locations, std::move(loc));
					}
					project.addLocations(std::move(locations));
				}

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
		environment.initialize(paths);

		for (auto& target : targets)
		{
			target->initialize();

			if (target->isProject())
			{
				paths.populateFileList(static_cast<ProjectTarget&>(*target));
			}
		}

		initializeCache();
	}

	if (!validateState())
		return false;

	if (!isConfigure)
		Diagnostic::printDone(timer.asString());

	Output::setShowCommandOverride(true);

	return true;
}

/*****************************************************************************/
void BuildState::initializeCache()
{
	m_prototype.cache.file().checkIfThemeChanged();

	m_prototype.cache.saveSettings(SettingsType::Local);
	m_prototype.cache.saveSettings(SettingsType::Global);
}

/*****************************************************************************/
bool BuildState::validateState()
{
	{
		auto workingDirectory = Commands::getWorkingDirectory();
		Path::sanitize(workingDirectory, true);

		if (String::toLowerCase(m_inputs.workingDirectory()) != String::toLowerCase(workingDirectory))
		{
			if (!Commands::changeWorkingDirectory(m_inputs.workingDirectory()))
			{
				Diagnostic::error("Error changing directory to '{}'", m_inputs.workingDirectory());
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
			Diagnostic::error("{} was either not defined in the cache, or not found.", makeExec.empty() ? "make" : makeExec);
			return false;
		}
	}
	else if (strat == StrategyType::Ninja)
	{
		toolchain.fetchNinjaVersion();

		auto& ninjaExec = toolchain.ninja();
		if (ninjaExec.empty() || !Commands::pathExists(ninjaExec))
		{
			Diagnostic::error("{} was either not defined in the cache, or not found.", ninjaExec.empty() ? "ninja" : ninjaExec);
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
			Diagnostic::error("The path to the CMake executable could not be resolved: {}", toolchain.cmake());
			return false;
		}
	}
	if (hasSubChaletTargets)
	{
		if (!m_prototype.tools.resolveOwnExecutable(m_inputs.appPath()))
		{
			Diagnostic::error("(Welp.) The path to the chalet executable could not be resolved: {}", m_prototype.tools.chalet());
			return false;
		}
	}

	for (auto& target : targets)
	{
		// must validate after cmake/sub-chalet check
		if (!target->validate())
		{
			Diagnostic::error("Error validating the '{}' target.", target->name());
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
void BuildState::manageLibraryPathVariables()
{
	Environment::set("CLICOLOR_FORCE", "1");
	Environment::set("CLANG_FORCE_COLOR_DIAGNOSTICS", "1");

#if defined(CHALET_LINUX)
	// Linux uses LD_LIBRARY_PATH & LIBRARY_PATH to resolve the correct file dependencies at runtime
	// Note: Not needed on mac: @rpath stuff is done instead

	// TODO: This might actually vary between distros? Test as many as possible

	auto addEnvironmentPath = [this](const char* inKey) {
		auto path = Environment::getAsString(inKey);
		auto outPath = environment.makePathVariable(path);
		if (outPath.size() > path.size())
		{
			// LOG(inKey, outPath);
			Environment::set(inKey, outPath);
		}
	};

	addEnvironmentPath("LD_LIBRARY_PATH");
	addEnvironmentPath("LIBRARY_PATH");

#endif
}

/*****************************************************************************/
void BuildState::enforceArchitectureInPath()
{
// works, but probably not a good idea
//
#if defined(CHALET_WIN32)
	Arch::Cpu targetArch = info.targetArchitecture();
	auto toolchainType = m_inputs.toolchainPreference().type;
	auto path = Environment::getPath();
	if (toolchainType == ToolchainType::GNU)
	{
		if (String::contains({ "\\mingw64\\", "\\mingw32\\" }, path))
		{
			std::string lower = String::toLowerCase(path);
			if (targetArch == Arch::Cpu::X64)
			{
				auto start = lower.find("\\mingw32\\");
				if (start != std::string::npos)
				{
					String::replaceAll(path, path.substr(start, 9), "\\mingw64\\");
					Environment::setPath(path);
				}
			}
			else if (targetArch == Arch::Cpu::X86)
			{
				auto start = lower.find("\\mingw64\\");
				if (start != std::string::npos)
				{
					String::replaceAll(path, path.substr(start, 9), "\\mingw32\\");
					Environment::setPath(path);
				}
			}
		}
	}
	else if (toolchainType == ToolchainType::LLVM)
	{
		if (String::contains({ "\\clang64\\", "\\clang32\\" }, path))
		{
			// TODO: clangarm64

			std::string lower = String::toLowerCase(path);
			if (targetArch == Arch::Cpu::X64)
			{
				auto start = lower.find("\\clang32\\");
				if (start != std::string::npos)
				{
					String::replaceAll(path, path.substr(start, 9), "\\clang64\\");
					Environment::setPath(path);
				}
			}
			else if (targetArch == Arch::Cpu::X86)
			{
				auto start = lower.find("\\clang64\\");
				if (start != std::string::npos)
				{
					String::replaceAll(path, path.substr(start, 9), "\\clang32\\");
					Environment::setPath(path);
				}
			}
		}
	}
#endif
}
}
