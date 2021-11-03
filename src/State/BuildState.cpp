/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildState.hpp"

#include "BuildJson/BuildJsonParser.hpp"
#include "Builder/BuildManager.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "SettingsJson/SettingsToolchainJsonParser.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/CompilerTools.hpp"
#include "State/StatePrototype.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
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
struct BuildState::Impl
{
	const CommandLineInputs inputs;
	StatePrototype& prototype;

	BuildInfo info;
	WorkspaceEnvironment workspace;
	CompilerTools toolchain;
	BuildPaths paths;
	BuildConfiguration configuration;
	std::vector<BuildTarget> targets;

	Unique<ICompileEnvironment> environment;

	bool checkForEnvironment = false;

	Impl(CommandLineInputs&& inInputs, StatePrototype& inPrototype, BuildState& inState) :
		inputs(std::move(inInputs)),
		prototype(inPrototype),
		info(inputs),
		workspace(prototype.workspace), // copy
		paths(inputs, inState)
	{
	}
};

/*****************************************************************************/
BuildState::BuildState(CommandLineInputs&& inInputs, StatePrototype& inPrototype) :
	m_impl(std::make_unique<Impl>(std::move(inInputs), inPrototype, *this)),
	tools(m_impl->prototype.tools),
	cache(m_impl->prototype.cache),
	info(m_impl->info),
	workspace(m_impl->workspace),
	toolchain(m_impl->toolchain),
	paths(m_impl->paths),
	configuration(m_impl->configuration),
	targets(m_impl->targets)
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

	if (m_impl->inputs.route() != Route::Configure)
	{
		if (!initializeBuild())
			return false;

		// calls enforceArchitectureInPath 2nd time
		makePathVariable();

		makeCompilerDiagnosticsVariables();
		makeLibraryPathVariables();
	}
	else
	{
		auto& cacheFile = m_impl->prototype.cache.file();
		m_uniqueId = getUniqueIdForState();
		cacheFile.setSourceCache(m_uniqueId, true);

		if (!initializeToolchain())
			return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildState::doBuild(const bool inShowSuccess)
{
	BuildManager mgr(m_impl->inputs, *this);
	return mgr.run(m_impl->inputs.route(), inShowSuccess);
}

bool BuildState::doBuild(const Route inRoute, const bool inShowSuccess)
{
	BuildManager mgr(m_impl->inputs, *this);
	return mgr.run(inRoute, inShowSuccess);
}

const std::string& BuildState::uniqueId() const noexcept
{
	return m_uniqueId;
}

/*****************************************************************************/
bool BuildState::initializeBuildConfiguration()
{
	auto buildConfiguration = m_impl->inputs.buildConfiguration();
	if (buildConfiguration.empty())
	{
		buildConfiguration = m_impl->prototype.anyConfiguration();
	}

	const auto& buildConfigurations = m_impl->prototype.buildConfigurations();

	if (buildConfigurations.find(buildConfiguration) == buildConfigurations.end())
	{
		Diagnostic::error("{}: The build configuration '{}' was not found.", m_impl->inputs.inputFile(), buildConfiguration);
		return false;
	}

	configuration = buildConfigurations.at(buildConfiguration);
	info.setBuildConfiguration(buildConfiguration);

	return true;
}

/*****************************************************************************/
bool BuildState::parseToolchainFromSettingsJson()
{
	auto createEnvironment = [this]() {
		m_impl->environment = ICompileEnvironment::make(m_impl->inputs.toolchainPreference().type, m_impl->inputs, *this);
		if (m_impl->environment == nullptr)
		{
			Diagnostic::error("The environment must be created when the toolchain is initialized.");
			return false;
		}

		if (!m_impl->environment->create(toolchain.version()))
			return false;

		return true;
	};

	m_impl->checkForEnvironment = false;
	auto& preference = m_impl->inputs.toolchainPreference();
	if (preference.type != ToolchainType::Unknown)
	{
		if (!createEnvironment())
			return false;
	}
	else
	{
		m_impl->checkForEnvironment = true;
	}

	auto& cacheFile = m_impl->prototype.cache.getSettings(SettingsType::Local);
	SettingsToolchainJsonParser parser(m_impl->inputs, *this, cacheFile);
	if (!parser.serialize())
		return false;

	ToolchainType type = ICompileEnvironment::detectToolchainTypeFromPath(toolchain.compilerCxxAny().path);
	if (preference.type != ToolchainType::Unknown && preference.type != type)
	{
		Diagnostic::error("Could not find a suitable toolchain that matches '{}'. Try configuring one manually, or ensuring the compiler is searchable from {}.", m_impl->inputs.toolchainPreferenceName(), Environment::getPathKey());
		return false;
	}

	if (m_impl->checkForEnvironment)
	{
		if (!createEnvironment())
			return false;
	}

	if (toolchain.version().empty())
		toolchain.setVersion(m_impl->environment->detectedVersion());

	if (!parser.validatePaths())
		return false;

	Output::setShowCommandOverride(false);

	if (!m_impl->environment->verifyToolchain())
	{
		Diagnostic::error("Unimplemented or unknown compiler toolchain.");
		return false;
	}

	environment = m_impl->environment.get();

	Output::setShowCommandOverride(true);

	return true;
}

/*****************************************************************************/
bool BuildState::parseBuildJson()
{
	BuildJsonParser parser(m_impl->inputs, m_impl->prototype, *this);
	return parser.serialize();
}

/*****************************************************************************/
bool BuildState::initializeToolchain()
{
	Timer timer;

	auto onError = [this]() -> bool {
		const auto& targetArch = m_impl->environment->type() == ToolchainType::GNU ?
			  m_impl->inputs.targetArchitecture() :
			  info.targetArchitectureTriple();

		if (!targetArch.empty())
		{
			Output::lineBreak();
			auto& toolchainName = m_impl->inputs.toolchainPreferenceName();
			Diagnostic::error("Architecture '{}' is not supported by the '{}' toolchain.", targetArch, toolchainName);
		}
		return false;
	};

	if (!m_impl->environment->makeArchitectureAdjustments())
		return onError();

	if (!toolchain.initialize(*m_impl->environment))
		return onError();

	if (!cache.updateSettingsFromToolchain(m_impl->inputs, toolchain))
		return false;

	return true;
}

/*****************************************************************************/
bool BuildState::initializeBuild()
{
	Timer timer;

	Output::setShowCommandOverride(false);

	Diagnostic::infoEllipsis("Initializing");

	auto& cacheFile = m_impl->prototype.cache.file();
	m_uniqueId = getUniqueIdForState(); // this will be incomplete by this point, but wee need it when the toolchain initializes
	cacheFile.setSourceCache(m_uniqueId, true);

	if (!initializeToolchain())
		return false;

	if (!paths.initialize())
		return false;

	for (auto& target : targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<SourceTarget&>(*target);
			project.parseOutputFilename();

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

			const bool isMsvc = environment->isMsvc();
			if (!isMsvc)
			{
				auto& compilerInfo = toolchain.compilerCxx(project.language());
				std::string libDir = compilerInfo.libDir;
				project.addLibDir(std::move(libDir));

				std::string includeDir = compilerInfo.includeDir;
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

	if (!info.initialize())
		return false;

	if (!validateSigningIdentity())
		return false;

	m_uniqueId = getUniqueIdForState();
	cacheFile.setSourceCache(m_uniqueId, toolchain.strategy() == StrategyType::Native);

	Diagnostic::printDone(timer.asString());

	Output::setShowCommandOverride(true);

	return true;
}

/*****************************************************************************/
void BuildState::initializeCache()
{
	m_impl->prototype.cache.file().checkIfThemeChanged();

	m_impl->prototype.cache.saveSettings(SettingsType::Local);
	m_impl->prototype.cache.saveSettings(SettingsType::Global);
}

/*****************************************************************************/
bool BuildState::validateState()
{
	auto workingDirectory = Commands::getWorkingDirectory();
	Path::sanitize(workingDirectory, true);

	if (String::toLowerCase(m_impl->inputs.workingDirectory()) != String::toLowerCase(workingDirectory))
	{
		if (!Commands::changeWorkingDirectory(m_impl->inputs.workingDirectory()))
		{
			Diagnostic::error("Error changing directory to '{}'", m_impl->inputs.workingDirectory());
			return false;
		}
	}

	if (!toolchain.validate())
		return false;

	auto& cacheFile = m_impl->prototype.cache.file();

	auto strat = toolchain.strategy();
	if (strat == StrategyType::Makefile)
	{
		const auto& makeExec = toolchain.make();
		if (makeExec.empty() || !Commands::pathExists(makeExec))
		{
			Diagnostic::error("{} was either not defined in the cache, or not found.", makeExec.empty() ? "make" : makeExec);
			return false;
		}

		toolchain.fetchMakeVersion(cacheFile.sources());

		for (auto& target : targets)
		{
			if (target->isProject())
			{
				auto& project = static_cast<SourceTarget&>(*target);

				if (project.language() != CodeLanguage::C && project.language() != CodeLanguage::CPlusPlus)
					continue;

#if defined(CHALET_WIN32)
				if (environment->isMsvc() && !toolchain.makeIsNMake())
				{
					Diagnostic::error("If using the 'makefile' strategy alongside MSVC, only NMake or Qt Jom are supported (found GNU make).");
					return false;
				}
				/*else if (environment->isMingwGcc() && toolchain.makeIsNMake())
				{
					Diagnostic::error("If using the 'makefile' strategy alongside MinGW, only GNU make is suported (found NMake or Qt Jom).");
					return false;
				}*/
#endif
				if (!environment->isAppleClang() && project.objectiveCxx())
				{
					Diagnostic::error("{}: Objective-C / Objective-C++ is currently only supported on MacOS using Apple clang. Use either 'language.macos' or '\"condition\": \"macos\"' in the '{}' project.", m_impl->inputs.inputFile(), project.name());
					return false;
				}
				break;
			}
		}
	}
	else if (strat == StrategyType::Ninja)
	{
		auto& ninjaExec = toolchain.ninja();
		if (ninjaExec.empty() || !Commands::pathExists(ninjaExec))
		{
			Diagnostic::error("{} was either not defined in the cache, or not found.", ninjaExec.empty() ? "ninja" : ninjaExec);
			return false;
		}

		toolchain.fetchNinjaVersion(cacheFile.sources());
	}

	bool hasCMakeTargets = false;
	bool hasSubChaletTargets = false;
	for (auto& target : targets)
	{
		hasSubChaletTargets |= target->isSubChalet();
		hasCMakeTargets |= target->isCMake();
	}

	if (hasCMakeTargets && !toolchain.fetchCmakeVersion(cacheFile.sources()))
	{
		Diagnostic::error("The path to the CMake executable could not be resolved: {}", toolchain.cmake());
		return false;
	}

	if (hasSubChaletTargets && !m_impl->prototype.tools.resolveOwnExecutable(m_impl->inputs.appPath()))
	{
		Diagnostic::error("(Welp.) The path to the chalet executable could not be resolved: {}", m_impl->prototype.tools.chalet());
		return false;
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

#if defined(CHALET_MACOS)
	if (configuration.enableProfiling())
		m_impl->prototype.tools.fetchXcodeVersion();
#endif

	return true;
}

/*****************************************************************************/
bool BuildState::validateSigningIdentity()
{
	// Right now, only used w/ Bundle
	if (m_impl->inputs.route() == Route::Bundle)
	{
		if (!tools.isSigningIdentityValid())
			return false;
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

	if (auto ccRoot = String::getPathFolder(toolchain.compilerC().path); !List::contains(pathList, ccRoot))
		outList.emplace_back(std::move(ccRoot));

	if (auto cppRoot = String::getPathFolder(toolchain.compilerCpp().path); !List::contains(pathList, cppRoot))
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

	bool usesGcc = environment->isGcc();
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

	if (m_impl->inputs.toolchainPreference().type != ToolchainType::VisualStudio)
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

/*****************************************************************************/
std::string BuildState::getUniqueIdForState() const
{
	std::string ret;
	const auto& hostArch = info.hostArchitectureTriple();
	const auto targetArch = m_impl->inputs.getArchWithOptionsAsString(info.targetArchitectureTriple());
	const auto& toolchainPref = m_impl->inputs.toolchainPreferenceName();
	const auto& strategy = toolchain.strategyString();
	const auto& buildConfig = info.buildConfiguration();
	const auto extensions = String::join(paths.allFileExtensions(), '_');

	std::string showCmds{ "0" };
	if (toolchain.strategy() != StrategyType::Ninja)
	{
		showCmds = std::to_string(Output::showCommands() ? 1 : 0);
	}

	ret = fmt::format("{}_{}_{}_{}_{}_{}_{}", hostArch, targetArch, toolchainPref, strategy, buildConfig, showCmds, extensions);

	return Hash::string(ret);
}

}
