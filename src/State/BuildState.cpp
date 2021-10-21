/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildState.hpp"

#include "BuildJson/BuildJsonParser.hpp"
#include "Builder/BuildManager.hpp"
#include "SettingsJson/SettingsToolchainJsonParser.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/AncillaryTools.hpp"
#include "State/StatePrototype.hpp"
#include "State/Target/SourceTarget.hpp"
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
BuildState::BuildState(CommandLineInputs inInputs, StatePrototype& inStatePrototype) :
	m_inputs(std::move(inInputs)),
	m_prototype(inStatePrototype),
	tools(m_prototype.tools),
	distribution(m_prototype.distribution),
	cache(m_prototype.cache),
	externalDependencies(m_prototype.externalDependencies),
	info(m_inputs),
	workspace(m_prototype.workspace), // copy
	toolchain(m_inputs, *this),
	paths(m_inputs, m_prototype.workspace)
{
}

/*****************************************************************************/
BuildState::~BuildState() = default;

/*****************************************************************************/
bool BuildState::initialize()
{
	// For now, enforceArchitectureInPath needs to be called before & after configuring the toolchain
	// Before: For when the toolchain & architecture are provided by inputs,
	//   and the toolchain needs to be populated into .chaletrc
	// After, for cases when the architecture was deduced after reading the cache
	enforceArchitectureInPath();

	if (!parseToolchainFromSettingsJson())
		return false;

	if (!initializeBuildConfiguration())
		return false;

	if (!parseBuildJson())
		return false;

	if (!initializeBuild())
		return false;

	// calls enforceArchitectureInPath 2nd time
	makePathVariable();

	makeCompilerDiagnosticsVariables();
	makeLibraryPathVariables();

	return true;
}

/*****************************************************************************/
bool BuildState::initializeForConfigure()
{
	enforceArchitectureInPath();

	if (!parseToolchainFromSettingsJson())
		return false;

	if (!parseBuildJson())
		return false;

	if (!initializeToolchain())
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
	if (!parser.serialize())
		return false;

	chalet_assert(environment != nullptr, "environment must be created when the toolchain is initialized.");
	return true;
}

/*****************************************************************************/
bool BuildState::parseBuildJson()
{
	BuildJsonParser parser(m_inputs, m_prototype, *this);
	return parser.serialize();
}

/*****************************************************************************/
bool BuildState::initializeToolchain()
{
	auto& settingsFile = m_prototype.cache.getSettings(SettingsType::Local);

	if (!toolchain.initialize(targets, settingsFile))
	{
		const auto& targetArch = m_inputs.toolchainPreference().type == ToolchainType::GNU ?
			  m_inputs.targetArchitecture() :
			  info.targetArchitectureString();

		if (!targetArch.empty())
		{
			auto& toolchainName = m_inputs.toolchainPreferenceName();
			Diagnostic::error("Requested arch '{}' is not supported by the '{}' toolchain.", targetArch, toolchainName);
		}
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildState::initializeBuild()
{
	Timer timer;

	Output::setShowCommandOverride(false);

	Diagnostic::infoEllipsis("Initializing");

	if (!tools.validateSigningIdentity())
		return false;

	if (!initializeToolchain())
		return false;

	if (!paths.initialize(info, toolchain))
		return false;

	for (auto& target : targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<SourceTarget&>(*target);
			auto& compilerConfig = toolchain.getConfig(project.language());
			project.parseOutputFilename(compilerConfig);

			if (!project.isStaticLibrary())
			{
				if (project.files().size() > 0)
				{
					StringList locations;
					for (auto& file : project.files())
					{
						List::addIfDoesNotExist(locations, String::getPathFolder(file));
					}
					project.addLocations(std::move(locations));
				}

#if defined(CHALET_WIN32) || defined(CHALET_LINUX)
				std::string intermediateDir = paths.intermediateDir();
				project.addLocation(std::move(intermediateDir));
#endif
			}

			const bool isMsvc = compilerConfig.isMsvc();
			if (!isMsvc)
			{
				std::string libDir = compilerConfig.compilerPathLib();
				project.addLibDir(std::move(libDir));

				std::string includeDir = compilerConfig.compilerPathInclude();
				project.addIncludeDir(std::move(includeDir));
			}

#if defined(CHALET_MACOS)
			project.addMacosFrameworkPath("/Library/Frameworks");
			project.addMacosFrameworkPath("/System/Library/Frameworks");
#endif

			for (auto& t : targets)
			{
				if (t->isProject())
				{
					auto& p = static_cast<SourceTarget&>(*t);
					bool staticLib = p.kind() == ProjectKind::StaticLibrary;
					project.resolveLinksFromProject(p.name(), staticLib);
				}
			}
		}
	}

	{
		workspace.initialize(paths);

		for (auto& target : targets)
		{
			target->initialize();

			if (target->isProject())
			{
				paths.populateFileList(static_cast<SourceTarget&>(*target));
			}
		}

		initializeCache();
	}

	if (!validateState())
		return false;

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

		for (auto& target : targets)
		{
			if (target->isProject())
			{
				auto& project = static_cast<SourceTarget&>(*target);

				if (project.language() != CodeLanguage::C && project.language() != CodeLanguage::CPlusPlus)
					continue;

				auto& compilerConfig = toolchain.getConfig(project.language());
#if defined(CHALET_WIN32)
				if (compilerConfig.isMsvc() && !toolchain.makeIsNMake())
				{
					Diagnostic::error("If using the 'makefile' strategy alongside MSVC, only NMake or Qt Jom are supported (found GNU make).");
					return false;
				}
				/*else if (compilerConfig.isMingwGcc() && toolchain.makeIsNMake())
				{
					Diagnostic::error("If using the 'makefile' strategy alongside MinGW, only GNU make is suported (found NMake or Qt Jom).");
					return false;
				}*/
#endif
				if (!compilerConfig.isAppleClang() && project.objectiveCxx())
				{
					Diagnostic::error("{}: Objective-C / Objective-C++ is currently only supported on MacOS using Apple clang. Use either 'language.macos' or '\"condition\": \"macos\"' in the '{}' project.", m_inputs.inputFile(), project.name());
					return false;
				}
				break;
			}
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
bool BuildState::makePathVariable()
{
	auto originalPath = Environment::getPath();
	Path::sanitize(originalPath);

	char separator = Path::getSeparator();
	auto pathList = String::split(originalPath, separator);

	StringList outList;

	if (auto ccRoot = String::getPathFolder(toolchain.compilerC()); !List::contains(pathList, ccRoot))
		outList.emplace_back(std::move(ccRoot));

	if (auto cppRoot = String::getPathFolder(toolchain.compilerCpp()); !List::contains(pathList, cppRoot))
		outList.emplace_back(std::move(cppRoot));

	for (auto& p : Path::getOSPaths())
	{
		if (!Commands::pathExists(p))
			continue;

		auto path = Commands::getCanonicalPath(p); // probably not needed, but just in case

		if (!List::contains(pathList, path))
			outList.emplace_back(std::move(path));
	}

	for (auto& path : pathList)
	{
		List::addIfDoesNotExist(outList, std::move(path));
	}

	std::string rootPath = String::join(std::move(outList), separator);
	Path::sanitize(rootPath);

	auto pathVariable = workspace.makePathVariable(rootPath);
	enforceArchitectureInPath(pathVariable);
	Environment::setPath(pathVariable);

	return true;
}

/*****************************************************************************/
void BuildState::makeCompilerDiagnosticsVariables()
{
	// Save the current environment to a file
	// std::system("printenv > all_variables.txt");

	Environment::set("CLICOLOR_FORCE", "1");
	Environment::set("CLANG_FORCE_COLOR_DIAGNOSTICS", "1");

	auto currentGccColors = Environment::getAsString("GCC_COLORS");
	if (!currentGccColors.empty())
		return;

	bool usesGcc = false;
	for (auto& target : targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<SourceTarget&>(*target);
			auto& config = toolchain.getConfig(project.language());
			if (config.isGcc())
				usesGcc = true;
		}
	}

	if (usesGcc)
	{
		std::string gccColors;
		const auto& theme = Output::theme();
		auto error = Output::getAnsiStyleRaw(theme.error);
		gccColors += fmt::format("error={}:", !error.empty() ? error : "01;31");
		auto warning = Output::getAnsiStyleRaw(theme.warning);
		gccColors += fmt::format("warning={}:", !warning.empty() ? warning : "01;33");
		auto note = Output::getAnsiStyleRaw(theme.note);
		gccColors += fmt::format("note={}:", !note.empty() ? note : "01;36");
		auto caret = Output::getAnsiStyleRaw(theme.success);
		gccColors += fmt::format("caret={}:", !caret.empty() ? caret : "01;32");
		auto locus = Output::getAnsiStyleRaw(theme.build);
		gccColors += fmt::format("locus={}:", !locus.empty() ? locus : "00;34");
		// auto quote = Output::getAnsiStyleRaw(theme.assembly);
		// gccColors += fmt::format("quote={}:", !quote.empty() ? quote : "01");
		gccColors += "quote=01";
		Environment::set("GCC_COLORS", gccColors);
	}
}

/*****************************************************************************/
void BuildState::makeLibraryPathVariables()
{
#if defined(CHALET_LINUX) || defined(CHALET_MACOS)
	// Linux uses LD_LIBRARY_PATH & LIBRARY_PATH to resolve the correct file dependencies at runtime

	if (workspace.searchPaths().empty())
		return;

	auto addEnvironmentPath = [this](const char* inKey) {
		auto path = Environment::getAsString(inKey);
		auto outPath = workspace.makePathVariable(path);
		// LOG(outPath);
		if (outPath != path)
		{
			// LOG(inKey, outPath);
			Environment::set(inKey, outPath);
		}
	};

	#if defined(CHALET_LINUX)
	addEnvironmentPath("LD_LIBRARY_PATH");
	addEnvironmentPath("LIBRARY_PATH");
	#elif defined(CHALET_MACOS)
	addEnvironmentPath("DYLD_FALLBACK_LIBRARY_PATH");
	addEnvironmentPath("DYLD_FALLBACK_FRAMEWORK_PATH");
	// DYLD_LIBRARY_PATH
	#endif
#endif
}

/*****************************************************************************/
void BuildState::enforceArchitectureInPath()
{
#if defined(CHALET_WIN32)
	auto path = Environment::getPath();
	enforceArchitectureInPath(path);
	Environment::setPath(path);
#endif
}

/*****************************************************************************/
void BuildState::enforceArchitectureInPath(std::string& outPathVariable)
{
	// Just common mingw conventions at the moment
	//
#if defined(CHALET_WIN32)
	Arch::Cpu targetArch = info.targetArchitecture();
	auto toolchainType = m_inputs.toolchainPreference().type;

	if (toolchainType != ToolchainType::VisualStudio)
	{
		std::string lower = String::toLowerCase(outPathVariable);

		if (String::contains({ "\\mingw64\\", "\\mingw32\\", "\\clang64\\", "\\clang32\\" }, outPathVariable))
		{
			// TODO: clangarm64

			if (targetArch == Arch::Cpu::X64)
			{
				auto start = lower.find("\\mingw32\\");
				if (start != std::string::npos)
				{
					String::replaceAll(outPathVariable, outPathVariable.substr(start, 9), "\\mingw64\\");
				}

				start = lower.find("\\clang32\\");
				if (start != std::string::npos)
				{
					String::replaceAll(outPathVariable, outPathVariable.substr(start, 9), "\\clang64\\");
				}
			}
			else if (targetArch == Arch::Cpu::X86)
			{
				auto start = lower.find("\\mingw64\\");
				if (start != std::string::npos)
				{
					String::replaceAll(outPathVariable, outPathVariable.substr(start, 9), "\\mingw32\\");
				}

				start = lower.find("\\clang64\\");
				if (start != std::string::npos)
				{
					String::replaceAll(outPathVariable, outPathVariable.substr(start, 9), "\\clang32\\");
				}
			}
		}
	}
#else
	UNUSED(outPathVariable);
#endif
}
}
