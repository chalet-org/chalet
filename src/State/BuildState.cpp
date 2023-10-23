/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildState.hpp"

#include "Builder/BuildManager.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "ChaletJson/ChaletJsonParser.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Core/DotEnvFileGenerator.hpp"
#include "Core/DotEnvFileParser.hpp"
#include "Export/IProjectExporter.hpp"
#include "SettingsJson/ToolchainSettingsJsonParser.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/CentralState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Dependency/IExternalDependency.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/IDistTarget.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonValues.hpp"

namespace chalet
{
struct BuildState::Impl
{
	const CommandLineInputs inputs;
	CentralState& centralState;

	BuildInfo info;
	// WorkspaceEnvironment workspace;
	CompilerTools toolchain;
	BuildPaths paths;
	BuildConfiguration configuration;
	std::vector<BuildTarget> targets;
	std::vector<DistTarget> distribution;

	Unique<ICompileEnvironment> environment;

	bool checkForEnvironment = false;

	Impl(CommandLineInputs&& inInputs, CentralState& inCentralState, BuildState& inState) :
		inputs(std::move(inInputs)),
		centralState(inCentralState),
		info(inState, inputs),
		// workspace(centralState.workspace), // copy
		paths(inState)
	{
	}
};

/*****************************************************************************/
BuildState::BuildState(CommandLineInputs inInputs, CentralState& inCentralState) :
	m_impl(std::make_unique<Impl>(std::move(inInputs), inCentralState, *this)),
	tools(m_impl->centralState.tools),
	cache(m_impl->centralState.cache),
	info(m_impl->info),
	workspace(m_impl->centralState.workspace),
	toolchain(m_impl->toolchain),
	paths(m_impl->paths),
	configuration(m_impl->configuration),
	targets(m_impl->targets),
	distribution(m_impl->distribution),
	inputs(m_impl->inputs),
	externalDependencies(m_impl->centralState.externalDependencies)
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

	if (!initializeToolchain())
		return false;

	if (!parseChaletJson())
		return false;

	if (!m_impl->centralState.isAllowedArchitecture(info.targetArchitectureTriple()))
		return false;

	// Update settings after toolchain & chalet.json have been parsed
	if (!cache.updateSettingsFromToolchain(inputs, m_impl->centralState, toolchain))
		return false;

	if (!initializeBuild())
		return false;

	if (!validateDistribution())
		return false;

	if (!validateState())
		return false;

	// calls enforceArchitectureInPath 2nd time
	makePathVariable();

	makeCompilerDiagnosticsVariables();

	return true;
}

/*****************************************************************************/
bool BuildState::generateProjects()
{
#if defined(CHALET_WIN32)
	if (!inputs.route().isClean() && toolchain.strategy() == StrategyType::MSBuild)
	{
		auto projectExporter = IProjectExporter::make(ExportKind::VisualStudioSolution, inputs);
		if (!projectExporter->generate(m_impl->centralState, true))
			return false;
	}
#elif defined(CHALET_MACOS)
	if (!inputs.route().isClean() && toolchain.strategy() == StrategyType::XcodeBuild)
	{
		auto projectExporter = IProjectExporter::make(ExportKind::Xcode, inputs);
		if (!projectExporter->generate(m_impl->centralState, true))
			return false;
	}
#endif

	return true;
}

/*****************************************************************************/
bool BuildState::doBuild(const CommandRoute& inRoute, const bool inShowSuccess)
{
	if (!generateProjects())
		return false;

	BuildManager mgr(*this);
	return mgr.run(inRoute, inShowSuccess);
}

/*****************************************************************************/
void BuildState::setCacheEnabled(const bool inValue)
{
	m_cacheEnabled = inValue;
}

/*****************************************************************************/
const std::string& BuildState::cachePathId() const noexcept
{
	return m_cachePathId;
}

/*****************************************************************************/
CentralState& BuildState::getCentralState()
{
	return m_impl->centralState;
}

/*****************************************************************************/
const CentralState& BuildState::getCentralState() const
{
	return m_impl->centralState;
}

/*****************************************************************************/
void BuildState::getTargetDependencies(StringList& outList, const std::string& inTargetName, const bool inWithSelf) const
{
	for (auto& target : targets)
	{
		bool isSources = target->isSources();
		bool isTarget = String::equals(inTargetName, target->name());
		if (isSources && isTarget)
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			for (auto& link : project.projectSharedLinks())
			{
				if (List::addIfDoesNotExist(outList, link))
					getTargetDependencies(outList, link, true);
			}

			for (auto& link : project.projectStaticLinks())
			{
				if (List::addIfDoesNotExist(outList, link))
					getTargetDependencies(outList, link, true);
			}
		}

		if (isTarget)
			break;

		if (!isSources && !inWithSelf)
		{
			List::addIfDoesNotExist(outList, target->name());
		}
	}

	if (inWithSelf)
	{
		List::addIfDoesNotExist(outList, inTargetName);
	}
}

/*****************************************************************************/
const IBuildTarget* BuildState::getFirstValidRunTarget() const
{
	auto lastTarget = inputs.lastTarget();
	if (String::equals(Values::All, lastTarget) && !targets.empty())
		lastTarget.clear();

	bool lastTargetWasSet = !lastTarget.empty();
	for (auto& target : targets)
	{
		auto& name = target->name();
		if (lastTargetWasSet && !String::equals(name, lastTarget))
			continue;

		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (project.isExecutable())
				return target.get();
		}
		else if (target->isCMake())
		{
			auto& project = static_cast<const CMakeTarget&>(*target);
			if (!project.runExecutable().empty())
				return target.get();
		}
		else if (target->isScript())
		{
			return target.get();
		}
		else if (target->isProcess())
		{
			return target.get();
		}
	}

	return nullptr;
}

/*****************************************************************************/
bool BuildState::initializeBuildConfiguration()
{
	auto buildConfiguration = inputs.buildConfiguration();
	if (buildConfiguration.empty())
	{
		Diagnostic::error("{}: No build configuration was set.", inputs.inputFile());
		return false;
	}

	const auto& buildConfigurations = m_impl->centralState.buildConfigurations();
	if (buildConfigurations.empty())
	{
		Diagnostic::error("{}: There are no build configurations defined for the workspace, and the defaults have been disabled.", inputs.inputFile());
		return false;
	}

	if (buildConfigurations.find(buildConfiguration) == buildConfigurations.end())
	{
		auto defaultBuildConfigs = BuildConfiguration::getDefaultBuildConfigurationNames();
		if (List::contains(defaultBuildConfigs, buildConfiguration))
			Diagnostic::error("{}: The build configuration '{}' is disabled in this workspace.", inputs.inputFile(), buildConfiguration);
		else
			Diagnostic::error("{}: The build configuration '{}' was not found.", inputs.inputFile(), buildConfiguration);
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
		m_impl->environment = ICompileEnvironment::make(inputs.toolchainPreference().type, *this);
		if (m_impl->environment == nullptr)
		{
			const auto& toolchainName = inputs.toolchainPreferenceName();
			auto arch = inputs.getResolvedTargetArchitecture();
			Diagnostic::error("The toolchain '{}' (arch: {}) could either not be detected or was not defined in settings.", toolchainName, arch);
			return false;
		}

		if (!m_impl->environment->create(toolchain.version()))
			return false;

		return true;
	};

	m_impl->checkForEnvironment = false;
	auto& preference = inputs.toolchainPreference();
	if (preference.type != ToolchainType::Unknown)
	{
		if (!createEnvironment())
		{
			Diagnostic::error("Toolchain was not recognized.");
			return false;
		}
	}
	else
	{
		m_impl->checkForEnvironment = true;
	}

	auto& settingsFile = m_impl->centralState.cache.getSettings(SettingsType::Local);
	ToolchainSettingsJsonParser parser(*this, settingsFile);
	if (!parser.serialize())
		return false;

	ToolchainType type = ICompileEnvironment::detectToolchainTypeFromPath(toolchain.compilerCxxAny().path);
	if (preference.type != ToolchainType::Unknown && preference.type != type)
	{
		// TODO: If using intel clang on windows, and another clang.exe is found in Path, this gets triggered
		//
		const auto& name = inputs.toolchainPreferenceName();
		std::string details;
		if (String::equals("llvm", name))
		{
			details = "If the version of LLVM requires a suffix, include it in the preset name (ie. 'llvm-14').";
		}
#if defined(CHALET_WIN32)
		else if (preference.type == ToolchainType::MingwGNU)
		{
			details = "An installation of MinGW was expected to be present in the 'Path' environment variable - set directly or via a '.env' file.";
		}
	#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
		else if (preference.type == ToolchainType::IntelLLVM)
		{
			details = "The 'intel-llvm-*' preset requires oneAPI DPC++/C++ compiler to be installed, and the 'ONEAPI_ROOT' environment variable to be set.";
		}
	#endif
#endif

		Diagnostic::error("Could not find a suitable toolchain that matches '{}'. Try configuring one manually, or ensuring the compiler is searchable from {}. {}", name, Environment::getPathKey(), details);

		return false;
	}

	if (m_impl->checkForEnvironment)
	{
		if (!createEnvironment())
			return false;
	}

	if (m_impl->environment != nullptr && toolchain.version().empty())
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
bool BuildState::parseChaletJson()
{
	ChaletJsonParser parser(m_impl->centralState, *this);
	return parser.serialize();
}

/*****************************************************************************/
bool BuildState::initializeToolchain()
{
	Timer timer;

	if (m_cacheEnabled || !m_impl->centralState.cache.file().sourceCacheAvailable())
	{
		auto& cacheFile = m_impl->centralState.cache.file();
		generateUniqueIdForState(); // this will be incomplete by this point, but wee need it when the toolchain initializes
		cacheFile.setBuildHash(m_uniqueId, true);
		cacheFile.setSourceCache(m_cachePathId, StrategyType::None);
	}

	auto onError = [this]() -> bool {
		const auto& targetArch = m_impl->environment->type() == ToolchainType::GNU ?
			inputs.targetArchitecture() :
			info.targetArchitectureTriple();

		if (!targetArch.empty())
		{
			Output::lineBreak();
			auto& toolchainName = inputs.toolchainPreferenceName();
			if (inputs.isToolchainPreset())
				Diagnostic::error("Architecture '{}' is not supported by the detected '{}' toolchain.", targetArch, toolchainName);
			else
				Diagnostic::error("Architecture '{}' is not supported by the '{}' toolchain.", targetArch, toolchainName);
		}
		return false;
	};

	if (!m_impl->environment->readArchitectureTripleFromCompiler())
		return onError();

	if (!toolchain.initialize(*m_impl->environment))
		return onError();

	if (!configuration.validate(*this))
	{
		Diagnostic::error("The build configuration '{}' can not be built.", configuration.name());
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildState::initializeBuild()
{
	Timer timer;

	Output::setShowCommandOverride(false);

	Diagnostic::infoEllipsis("Configuring build");

	if (!info.initialize())
		return false;

	if (!paths.initialize())
		return false;

	// No longer needed
	if (!inputs.route().isExport())
		m_impl->centralState.clearRunArgumentMap();

	// These should only be relevant if cross-compiling (so far)
	//
	environment->generateTargetSystemPaths();

	// Get the path to windres, but with this method, it's not saved in settings
	//  - it's specific to this architecture that we're building for
	//
	if (environment->isClang() && info.targettingMinGW())
	{
		auto compilerPath = toolchain.compilerCxxAny().binDir;
		auto windres = fmt::format("{}/{}-windres", compilerPath, info.targetArchitectureTriple());
		if (Commands::pathExists(windres))
		{
			toolchain.setCompilerWindowsResource(std::move(windres));
		}
	}

	for (auto& target : targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<SourceTarget&>(*target);
			paths.setBuildDirectoriesBasedOnProjectKind(project);
			project.parseOutputFilename();

			auto objDir = paths.objDir();
			project.addIncludeDir(std::move(objDir));

			auto intermediateDir = paths.intermediateDir(project);
			project.addIncludeDir(std::move(intermediateDir));

			if (!inputs.route().isExport())
			{
				const bool isMsvc = environment->isMsvc();
				if (!isMsvc)
				{
					auto& compilerInfo = toolchain.compilerCxx(project.language());
					auto libDir = compilerInfo.libDir;
					project.addLibDir(std::move(libDir));

					auto includeDir = compilerInfo.includeDir;
					project.addIncludeDir(std::move(includeDir));
				}

#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
				const auto& systemPaths = environment->targetSystemPaths();
				if (systemPaths.empty())
				{
					std::string localLib{ "/usr/local/lib" };
					if (Commands::pathExists(localLib))
						project.addLibDir(std::move(localLib));

					std::string localInclude{ "/usr/local/include" };
					if (Commands::pathExists(localInclude))
						project.addIncludeDir(std::move(localInclude));
				}
#endif
#if defined(CHALET_MACOS)
				project.addAppleFrameworkPath("/Library/Frameworks");
				project.addAppleFrameworkPath("/System/Library/Frameworks");
#endif
			}

			if (!project.resolveLinksFromProject(targets, inputs.inputFile()))
				return false;
		}
	}

	{
		if (!workspace.initialize(*this))
			return false;

		for (auto& target : targets)
		{
			if (!target->initialize())
				return false;

			if (target->isSources())
			{
				paths.populateFileList(static_cast<SourceTarget&>(*target));
			}
		}

		for (auto& target : distribution)
		{
			if (!target->initialize())
				return false;
		}

		initializeCache();
	}

	if (m_cacheEnabled || !m_impl->centralState.cache.file().sourceCacheAvailable())
	{
		auto& cacheFile = m_impl->centralState.cache.file();
		generateUniqueIdForState();
		cacheFile.setBuildHash(m_uniqueId, false);
		cacheFile.setSourceCache(m_cachePathId, toolchain.strategy());
	}

	Diagnostic::printDone(timer.asString());

	Output::setShowCommandOverride(true);

	return true;
}

/*****************************************************************************/
void BuildState::initializeCache()
{
	m_impl->centralState.cache.saveSettings(SettingsType::Local);
	m_impl->centralState.cache.saveSettings(SettingsType::Global);

	std::string metadataTemp = workspace.metadata().getHash();
	for (auto& target : targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<SourceTarget&>(*target);
			if (project.hasMetadata())
			{
				metadataTemp += project.metadata().getHash();
			}
		}
	}

	auto metadataHash = Hash::string(metadataTemp);
	m_impl->centralState.cache.file().checkForMetadataChange(metadataHash);
}

/*****************************************************************************/
bool BuildState::validateState()
{
	auto workingDirectory = Commands::getWorkingDirectory();
	Path::sanitize(workingDirectory, true);

	if (String::toLowerCase(inputs.workingDirectory()) != String::toLowerCase(workingDirectory))
	{
		if (!Commands::changeWorkingDirectory(inputs.workingDirectory()))
		{
			Diagnostic::error("Error changing directory to '{}'", inputs.workingDirectory());
			return false;
		}
	}

	if (!info.validate())
		return false;

	if (!toolchain.validate())
		return false;

	const bool lto = configuration.interproceduralOptimization();
	if (lto && info.dumpAssembly() && !environment->isClang())
	{
#if defined(CHALET_WIN32)
		if (toolchain.strategy() != StrategyType::MSBuild)
#endif
		{
			Diagnostic::error("Enabling 'dumpAssembly' with the configuration '{}' is not possible because it uses interprocedural optimizations.", configuration.name());
			return false;
		}
	}

	for (auto& target : targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<SourceTarget&>(*target);
			if (project.cppModules())
			{
				if (project.language() != CodeLanguage::CPlusPlus)
				{
					Diagnostic::error("{}: C++ modules are only supported with C++. Found C target with 'modules' enabled.", inputs.inputFile());
					return false;
				}

				if (!environment->isMsvc())
				{
					Diagnostic::error("{}: C++ modules are only supported with MSVC.", inputs.inputFile());
					return false;
				}

				uint versionMajorMinor = toolchain.compilerCxx(project.language()).versionMajorMinor;
				if (versionMajorMinor < 1928)
				{
					Diagnostic::error("{}: C++ modules are only supported in Chalet with MSVC versions >= 19.28 (found {})", inputs.inputFile(), toolchain.compilerCxx(project.language()).version);
					return false;
				}

				if (project.objectiveCxx())
				{
					Diagnostic::error("{}: C++ modules are not supported alongside Objective-C++", inputs.inputFile());
					return false;
				}

				auto langStandard = project.cppStandard();
				String::replaceAll(langStandard, "gnu++", "");
				String::replaceAll(langStandard, "c++", "");

				if (langStandard.empty() || langStandard.front() != '2')
				{
					Diagnostic::error("{}: C++ modules are only supported with the c++20 standard or higher.", inputs.inputFile());
					return false;
				}
			}

			if (!environment->isAppleClang() && project.objectiveCxx())
			{
				Diagnostic::error("{}: Objective-C / Objective-C++ is currently only supported on MacOS using Apple clang.", inputs.inputFile());
				return false;
			}
		}
	}

	auto& cacheFile = m_impl->centralState.cache.file();
	auto strat = toolchain.strategy();
	if (strat == StrategyType::Makefile)
	{
		const auto& makeExec = toolchain.make();
		if (makeExec.empty() || !Commands::pathExists(makeExec))
		{
			Diagnostic::error("{} was either not defined in the toolchain, or not found.", makeExec.empty() ? "make" : makeExec);
			return false;
		}

#if defined(CHALET_WIN32)
		for (auto& target : targets)
		{
			if (target->isSources())
			{
				if ((environment->isMsvc() || environment->isMsvcClang()) && !toolchain.makeIsNMake())
				{
					Diagnostic::error("If using the 'makefile' strategy alongside MSVC, only NMake or Qt Jom are supported (found GNU make).");
					return false;
				}
				/*else if (environment->isMingwGcc() && toolchain.makeIsNMake())
				{
					Diagnostic::error("If using the 'makefile' strategy alongside MinGW, only GNU make is suported (found NMake or Qt Jom).");
					return false;
				}*/
				break; // note: break because we only care if there's any source targets
			}
		}
#endif
	}
	else if (strat == StrategyType::Ninja)
	{
		auto& ninjaExec = toolchain.ninja();
		if (ninjaExec.empty() || !Commands::pathExists(ninjaExec))
		{
			Diagnostic::error("{} was either not defined in the toolchain, or not found.", ninjaExec.empty() ? "ninja" : ninjaExec);
			return false;
		}
	}
	else if (strat == StrategyType::MSBuild)
	{
#if defined(CHALET_WIN32)
		if (!environment->isMsvc())
		{
			Diagnostic::error("The 'msbuild' strategy is only allowed with one of the VS toolchain presets.");
			return false;
		}
#else
		Diagnostic::error("The 'msbuild' strategy is only available on Windows.");
		return false;
#endif
	}
	else if (strat == StrategyType::XcodeBuild)
	{
#if defined(CHALET_MACOS)
		if (!environment->isAppleClang())
		{
			Diagnostic::error("The 'xcodebuild' strategy is only allowed with the apple-llvm preset.");
			return false;
		}

		if (Commands::isUsingAppleCommandLineTools())
		{
			Diagnostic::error("The 'xcodebuild' strategy cannot be used with CommandLineTools. Please run 'sudo xcode-select -s /Applications/Xcode.app/Contents/Developer' (or with your chosen path to Xcode)");
			return false;
		}
#else
		Diagnostic::error("The 'xcodebuild' strategy is only available on macOS.");
		return false;
#endif
	}

	toolchain.fetchMakeVersion(cacheFile.sources());
	toolchain.fetchNinjaVersion(cacheFile.sources());

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

	UNUSED(hasSubChaletTargets);
	if (!m_impl->centralState.tools.resolveOwnExecutable(inputs.appPath()))
		return false;

	// Note: Ignored in the clean command so targets with external dependency paths don't get validated
	//
	if (!inputs.route().isClean())
	{
		for (auto& target : targets)
		{
			// must validate after cmake/sub-chalet check
			if (!target->validate())
			{
				Diagnostic::error("Error validating the '{}' target.", target->name());
				return false;
			}
		}
	}

	if (configuration.enableProfiling() && !inputs.route().isExport())
	{
#if defined(CHALET_MACOS)
		bool profilerAvailable = true;
#else
		bool profilerAvailable = !toolchain.profiler().empty() && Commands::pathExists(toolchain.profiler());
#endif
		if (!profilerAvailable)
		{
			Diagnostic::error("The profiler for this toolchain was either blank or not found.");
			return false;
		}

		profilerAvailable = false;
#if defined(CHALET_MACOS)
		profilerAvailable |= environment->isAppleClang();
#elif defined(CHALET_WIN32)
		bool requiresVisualStudio = environment->isMsvc() && toolchain.isProfilerVSInstruments();
		profilerAvailable |= requiresVisualStudio;
#endif
		profilerAvailable |= toolchain.isProfilerGprof();
		if (!profilerAvailable)
		{
			Diagnostic::error("Profiling on this toolchain is not yet supported.");
			return false;
		}
#if defined(CHALET_WIN32)
		if (requiresVisualStudio)
		{
			auto vsperfcmd = Commands::which("vsperfcmd");
			if (vsperfcmd.empty())
			{
				std::string progFiles = Environment::getString("ProgramFiles(x86)");
				// TODO: more portable version
				vsperfcmd = fmt::format("{}\\Microsoft Visual Studio\\Shared\\Common\\VSPerfCollectionTools\\vs2022\\vsperfcmd.exe", progFiles);
				if (vsperfcmd.empty())
				{
					Diagnostic::error("Profiling with MSVC requires vsperfcmd.exe or vsperf.exe, but they were not found in Path.");
					return false;
				}
			}
			tools.setVsperfcmd(std::move(vsperfcmd));
		}
#endif

#if defined(CHALET_MACOS)
		m_impl->centralState.tools.fetchXcodeVersion();
#endif
	}

	// Right now, only used w/ Bundle
	if (inputs.route().isBundle())
	{
		if (!tools.isSigningIdentityValid())
			return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildState::validateDistribution()
{
	for (auto& target : distribution)
	{
		if (!target->validate())
		{
			Diagnostic::error("Error validating the '{}' distribution target.", target->name());
			return false;
		}
	}

	auto& distributionDirectory = inputs.distributionDirectory();

	Dictionary<std::string> locations;
	bool result = true;
	for (auto& target : distribution)
	{
		if (target->isDistributionBundle())
		{
			auto& bundle = static_cast<BundleTarget&>(*target);

			/*if (bundle.buildTargets().empty())
			{
				Diagnostic::error("{}: Distribution bundle '{}' was found without 'buildTargets'", m_filename, bundle.name());
				return false;
			}*/

			if (!distributionDirectory.empty())
			{
				auto& subdirectory = bundle.subdirectory();
				bundle.setSubdirectory(fmt::format("{}/{}", distributionDirectory, subdirectory));
			}

			for (auto& targetName : bundle.buildTargets())
			{
				auto res = locations.find(targetName);
				if (res != locations.end())
				{
					if (res->second == bundle.subdirectory())
					{
						Diagnostic::error("Project '{}' has duplicate bundle destination of '{}' defined in bundle: {}", targetName, bundle.subdirectory(), bundle.name());
						result = false;
					}
					else
					{
						locations.emplace(targetName, bundle.subdirectory());
					}
				}
				else
				{
					locations.emplace(targetName, bundle.subdirectory());
				}
			}
		}
	}

	return result;
}

/*****************************************************************************/
void BuildState::makePathVariable()
{
	auto originalPath = Environment::getPath();
	Path::sanitize(originalPath);

	char separator = Environment::getPathSeparator();
	auto pathList = String::split(originalPath, separator);

	StringList outList;

	if (auto ccRoot = String::getPathFolder(toolchain.compilerC().path); !List::contains(pathList, ccRoot))
		outList.emplace_back(std::move(ccRoot));

	if (auto cppRoot = String::getPathFolder(toolchain.compilerCpp().path); !List::contains(pathList, cppRoot))
		outList.emplace_back(std::move(cppRoot));

	{
		// Edge case for cross-compilers that have an extra bin folder (like MinGW on Linux)
		auto extraBinDir = fmt::format("{}/bin", String::getPathFolder(toolchain.compilerCpp().libDir));
		if (Commands::pathExists(extraBinDir))
		{
			if (!List::contains(pathList, extraBinDir))
				outList.emplace_back(std::move(extraBinDir));
		}
	}

#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
	StringList osPaths{
		"/usr/local/sbin",
		"/usr/local/bin",
		"/usr/sbin",
		"/usr/bin",
		"/sbin",
		"/bin",
	};

	for (auto& p : osPaths)
	{
		if (!Commands::pathExists(p))
			continue;

		auto path = Commands::getCanonicalPath(p); // probably not needed, but just in case

		if (!List::contains(pathList, path))
			outList.emplace_back(std::move(path));
	}
#endif

	for (auto& path : pathList)
	{
		List::addIfDoesNotExist(outList, std::move(path));
	}

	std::string rootPath = String::join(std::move(outList), separator);
	Path::sanitize(rootPath);

	auto pathVariable = workspace.makePathVariable(rootPath);
	enforceArchitectureInPath(pathVariable);
	Environment::setPath(pathVariable);
}

/*****************************************************************************/
void BuildState::makeCompilerDiagnosticsVariables()
{
	// Save the current environment to a file
	// std::system("printenv > all_variables.txt");

#if defined(CHALET_WIN32)
	// For python script & process targets
	Environment::set("PYTHONIOENCODING", "utf-8");
	Environment::set("PYTHONLEGACYWINDOWSSTDIO", "utf-8");
#endif

	Environment::set("CLICOLOR_FORCE", "1");
	Environment::set("CLANG_FORCE_COLOR_DIAGNOSTICS", "1");

	auto currentGccColors = Environment::getString("GCC_COLORS");
	if (currentGccColors.empty())
	{
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
}

/*****************************************************************************/
void BuildState::makeLibraryPathVariables()
{
	auto dotEnvGen = DotEnvFileGenerator::make(*this);

	auto filename = fmt::format("{}/run.env", paths.buildOutputDir());
	dotEnvGen.save(filename);

	DotEnvFileParser parser(inputs);
	parser.readVariablesFromFile(filename);
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
#if defined(CHALET_WIN32)
	Arch::Cpu targetArch = info.targetArchitecture();

	auto type = inputs.toolchainPreference().type;
	if (type == ToolchainType::VisualStudio)
		return;

	std::string lower = String::toLowerCase(outPathVariable);

	// If using Intel LLVM, add the compiler path (This might just work with the 2023 version)
	//
	if (type == ToolchainType::IntelLLVM)
	{
		auto oneApi = Environment::getString("ONEAPI_ROOT");
		if (!oneApi.empty())
		{
			Path::sanitizeForWindows(oneApi);
			oneApi = fmt::format("{}compiler\\latest\\windows\\bin-llvm", oneApi);
			std::string lowerOneApi = String::toLowerCase(oneApi);
			if (!String::contains(lowerOneApi, lower))
			{
				outPathVariable = fmt::format("{};{}", oneApi, outPathVariable);
			}
		}
	}
	// If using MinGW, search the most common install paths
	//   x64:
	//    C:/msys64/ucrt64 - recommended by the MSYS2 team
	//    C:/msys64/mingw64 - if ucrt64 is not available
	//    C:/mingw64
	//   x86:
	//    C:/msys64/mingw32
	//    C:/mingw32
	//
	else if (type == ToolchainType::MingwGNU)
	{
		// We only want to do this if the preference name was simply "gcc", and a "gcc.exe" was not found
		//   Other GCC toolchain variants (with prefixes/suffixes) should not assume anything
		//
		auto& preferenceName = inputs.toolchainPreferenceName();
		if (String::equals("gcc", preferenceName))
		{
			auto gcc = Commands::which("gcc");
			if (gcc.empty())
			{
				auto homeDrive = Environment::getString("HOMEDRIVE");
				if (!homeDrive.empty())
				{
					// Check for MSYS2 first
					//
					std::string mingwPath;
					auto msysPath = fmt::format("{}\\msys64", homeDrive);
					if (Commands::pathExists(msysPath))
					{
						if (targetArch == Arch::Cpu::X64)
						{
							// Favor the UCRT version if it's installed
							//
							mingwPath = fmt::format("{}\\ucrt64\\bin", msysPath);

							if (!Commands::pathExists(fmt::format("{}\\gcc.exe", mingwPath)))
							{
								mingwPath = fmt::format("{}\\mingw64\\bin", msysPath);
							}
						}
						else if (targetArch == Arch::Cpu::X86)
						{
							mingwPath = fmt::format("{}\\mingw32\\bin", msysPath);
						}
					}
					// Then check for C:/mingw64
					//
					else
					{
						if (targetArch == Arch::Cpu::X64)
						{
							mingwPath = fmt::format("{}\\mingw64\\bin", homeDrive);
						}
						else if (targetArch == Arch::Cpu::X86)
						{
							mingwPath = fmt::format("{}\\mingw32\\bin", msysPath);
						}
					}

					if (!Commands::pathExists(fmt::format("{}\\gcc.exe", mingwPath)))
					{
						mingwPath.clear();
					}

					if (!mingwPath.empty())
					{
						std::string lowerMinGwPath = String::toLowerCase(mingwPath);
						if (!String::contains(lowerMinGwPath, lower))
						{
							outPathVariable = fmt::format("{};{}", mingwPath, outPathVariable);
						}
					}
				}
			}
		}
	}
	// If using LLVM, detect it from Program Files if clang doesn't exist in Path
	//
	else if (type == ToolchainType::LLVM)
	{
		auto& preferenceName = inputs.toolchainPreferenceName();
		if (String::equals("llvm", preferenceName))
		{
			auto clang = Commands::which("clang");
			if (clang.empty())
			{
				auto programFiles = Environment::getString("ProgramFiles");
				if (!programFiles.empty())
				{
					auto clangPath = fmt::format("{}\\LLVM\\bin", programFiles);
					if (Commands::pathExists(fmt::format("{}\\clang.exe", clangPath)))
					{
						std::string lowerClangPath = String::toLowerCase(clangPath);
						if (!String::contains(lowerClangPath, lower))
						{
							outPathVariable = fmt::format("{};{}", clangPath, outPathVariable);
						}
					}
				}
			}
		}
	}

	// Common MinGW conventions
	//
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

		start = lower.find("\\clangarm64\\");
		if (start != std::string::npos)
		{
			String::replaceAll(outPathVariable, outPathVariable.substr(start, 12), "\\clang64\\");
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

		start = lower.find("\\clangarm64\\");
		if (start != std::string::npos)
		{
			String::replaceAll(outPathVariable, outPathVariable.substr(start, 12), "\\clang32\\");
		}
	}
	else if (targetArch == Arch::Cpu::ARM64)
	{
		auto start = lower.find("\\clang32\\");
		if (start != std::string::npos)
		{
			String::replaceAll(outPathVariable, outPathVariable.substr(start, 9), "\\clangarm64\\");
		}

		start = lower.find("\\clang64\\");
		if (start != std::string::npos)
		{
			String::replaceAll(outPathVariable, outPathVariable.substr(start, 9), "\\clangarm64\\");
		}
	}
#else
	UNUSED(outPathVariable);
#endif
}

/*****************************************************************************/
bool BuildState::replaceVariablesInString(std::string& outString, const IBuildTarget* inTarget, const bool inCheckHome, const std::function<std::string(std::string)>& onFail) const
{
	if (outString.empty())
		return true;

	if (inCheckHome)
	{
		const auto& homeDirectory = inputs.homeDirectory();
		Environment::replaceCommonVariables(outString, homeDirectory);
	}

	if (String::contains("${", outString))
	{
		if (!RegexPatterns::matchAndReplacePathVariables(outString, [this, &inTarget, &onFail](std::string match, bool& required) {
				if (String::equals("cwd", match))
					return inputs.workingDirectory();

				if (String::equals("architecture", match))
					return info.targetArchitectureString();

				if (String::equals("targetTriple", match))
					return info.targetArchitectureTriple();

				if (String::equals("configuration", match))
					return configuration.name();

				if (String::equals("buildDir", match))
					return paths.buildOutputDir();

				if (String::equals("home", match))
					return inputs.homeDirectory();

				if (inTarget != nullptr)
				{
					if (String::equals("name", match))
						return inTarget->name();
				}

				if (String::startsWith("meta:workspace", match))
				{
					required = false;
					match = match.substr(14);
					match[0] = static_cast<char>(::tolower(static_cast<uchar>(match[0])));

					const auto& metadata = workspace.metadata();
					return metadata.getMetadataFromString(match);
				}
				else if (String::startsWith("meta:", match))
				{
					required = false;
					match = match.substr(5);
					if (inTarget != nullptr && inTarget->isSources())
					{
						const auto& project = static_cast<const SourceTarget&>(*inTarget);
						if (project.hasMetadata())
						{
							const auto& metadata = project.metadata();
							return metadata.getMetadataFromString(match);
						}
					}

					const auto& metadata = workspace.metadata();
					return metadata.getMetadataFromString(match);
				}

				if (String::startsWith("env:", match))
				{
					required = false;
					match = match.substr(4);
					return Environment::getString(match.c_str());
				}

				if (String::startsWith("defined:", match))
				{
					required = false;
					match = match.substr(8);
					auto result = Environment::get(match.c_str());
					if (result == nullptr)
					{
						if (inTarget->isSources())
						{
							auto& project = static_cast<const SourceTarget&>(*inTarget);
							const auto& defines = project.defines();
							for (auto& define : defines)
							{
								if (String::equals(match, define))
									result = "1";
							}
						}
					}
					return std::string(result != nullptr ? "true" : "false");
				}

				if (String::startsWith("var:", match))
				{
					required = false;
					match = match.substr(4);
					return tools.variables.get(match);
				}

				if (String::startsWith("external:", match))
				{
					match = match.substr(9);
					auto val = paths.getExternalDir(match);
					if (val.empty())
					{
						Diagnostic::error("{}: External dependency '{}' does not exist.", inputs.inputFile(), match);
					}
					return val;
				}

				if (String::startsWith("externalBuild:", match))
				{
					match = match.substr(14);
					auto val = paths.getExternalBuildDir(match);
					if (val.empty())
					{
						Diagnostic::error("{}: External dependency '{}' does not exist.", inputs.inputFile(), match);
					}
					return val;
				}

				if (onFail != nullptr)
					return onFail(std::move(match));

				return std::string();
			}))
		{
			const auto& name = inTarget != nullptr ? inTarget->name() : std::string();
			Diagnostic::error("{}: Target '{}' has an unsupported variable in the value: {}", inputs.inputFile(), name, outString);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool BuildState::replaceVariablesInString(std::string& outString, const IDistTarget* inTarget, const bool inCheckHome, const std::function<std::string(std::string)>& onFail) const
{
	if (outString.empty())
		return true;

	if (inCheckHome)
	{
		const auto& homeDirectory = inputs.homeDirectory();
		Environment::replaceCommonVariables(outString, homeDirectory);
	}

	if (String::contains("${", outString))
	{
		if (!RegexPatterns::matchAndReplacePathVariables(outString, [this, &inTarget, &onFail](std::string match, bool& required) {
				if (String::equals("cwd", match))
					return inputs.workingDirectory();

				if (String::equals("architecture", match))
					return info.targetArchitectureString();

				if (String::equals("targetTriple", match))
					return info.targetArchitectureTriple();

				if (String::equals("configuration", match))
					return configuration.name();

				if (String::equals("buildDir", match))
					return paths.buildOutputDir();

				if (String::equals("distributionDir", match))
					return inputs.distributionDirectory();

				if (String::equals("home", match))
					return inputs.homeDirectory();

				if (inTarget != nullptr)
				{
					if (String::equals("name", match))
						return inTarget->name();
				}

				if (String::startsWith("meta:workspace", match))
				{
					required = false;
					match = match.substr(14);
					match[0] = static_cast<char>(::tolower(static_cast<uchar>(match[0])));

					const auto& metadata = workspace.metadata();
					return metadata.getMetadataFromString(match);
				}
				else if (String::startsWith("meta:", match))
				{
					match = match.substr(5);

					required = false;
					const auto& metadata = workspace.metadata();
					return metadata.getMetadataFromString(match);
				}

				if (String::startsWith("env:", match))
				{
					required = false;
					match = match.substr(4);
					return Environment::getString(match.c_str());
				}

				if (String::startsWith("defined:", match))
				{
					required = false;
					match = match.substr(8);
					auto result = Environment::get(match.c_str());
					return std::string(result != nullptr ? "true" : "false");
				}

				if (String::startsWith("var:", match))
				{
					required = false;
					match = match.substr(4);
					return tools.variables.get(match);
				}

				if (String::startsWith("external:", match))
				{
					match = match.substr(9);
					auto val = paths.getExternalDir(match);
					if (val.empty())
					{
						Diagnostic::error("{}: External dependency '{}' does not exist.", inputs.inputFile(), match);
					}
					return val;
				}

				if (String::startsWith("externalBuild:", match))
				{
					match = match.substr(14);
					auto val = paths.getExternalBuildDir(match);
					if (val.empty())
					{
						Diagnostic::error("{}: External dependency '{}' does not exist.", inputs.inputFile(), match);
					}
					return val;
				}

				if (onFail != nullptr)
					return onFail(std::move(match));

				return std::string();
			}))
		{
			const auto& name = inTarget != nullptr ? inTarget->name() : std::string();
			Diagnostic::error("{}: Distribution target '{}' has an unsupported variable in: {}", inputs.inputFile(), name, outString);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
void BuildState::generateUniqueIdForState()
{
	const auto& hostArch = info.hostArchitectureString();
	const auto targetArch = inputs.getArchWithOptionsAsString(info.targetArchitectureTriple());
	const auto envId = m_impl->environment->identifier() + toolchain.version();
	const auto& buildConfig = info.buildConfiguration();
	const auto& targetOsName = inputs.osTargetName();
	const auto& targetOsVersion = inputs.osTargetVersion();

	bool showCmds = false;
	if (toolchain.strategy() != StrategyType::Ninja)
	{
		showCmds = Output::showCommands();
	}

	bool dumpAssembly = false;
	/*if (m_impl->environment->type() == ToolchainType::VisualStudio)
	{
		dumpAssembly = info.dumpAssembly();
	}*/

	std::string targetHash;
	for (auto& target : targets)
	{
		targetHash += target->getHash();
	}

	// Note: no targetHash
	auto hashable = Hash::getHashableString(hostArch, targetArch, targetOsName, targetOsVersion, envId, buildConfig, showCmds, dumpAssembly);
	m_cachePathId = Hash::string(hashable);

	// Unique ID is used by the internal cache to determine if the build files need to be updated
	auto hashableTargets = Hash::getHashableString(m_cachePathId, targetHash);
	m_uniqueId = Hash::string(hashableTargets);
}
}
