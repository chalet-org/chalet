/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/CentralState.hpp"

#include <thread>

#include "ChaletJson/CentralChaletJsonParser.hpp"
#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Dependencies/DependencyManager.hpp"
#include "SettingsJson/GlobalSettingsJsonParser.hpp"
#include "SettingsJson/SettingsJsonParser.hpp"

#include "Core/Arch.hpp"
#include "Core/DotEnvFileParser.hpp"
#include "Core/QueryController.hpp"
#include "SettingsJson/IntermediateSettingsState.hpp"
#include "State/Dependency/LocalDependency.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/WindowsTerminal.hpp"
#include "Utility/List.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Utility/Version.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
CentralState::CentralState(CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
}

/*****************************************************************************/
bool CentralState::initialize()
{
#if defined(CHALET_WIN32)
	WindowsTerminal::initializeCreateProcess();
#endif

	const auto& route = m_inputs.route();
	chalet_assert(!route.isQuery(), "");

	Diagnostic::usePaddedErrors();

	if (!parseEnvFile())
		return false;

	if (!cache.initializeSettings(m_inputs))
		return false;

	// if (!workspace.initialize())
	// 	return false;

	{
		// Set global defaults here
		IntermediateSettingsState state;
		state.maxJobs = std::thread::hardware_concurrency();
		state.benchmark = true;
		state.launchProfiler = true;
		state.keepGoing = false;
		state.showCommands = false;
		state.dumpAssembly = false;
		state.generateCompileCommands = false;
		state.buildConfiguration = BuildConfiguration::getDefaultReleaseConfigurationName();
		state.toolchainPreference = m_inputs.defaultToolchainPreset();
		state.architecturePreference = "auto";
		state.inputFile = m_inputs.defaultInputFile();
		state.envFile = m_inputs.defaultEnvFile();
		// state.rootDirectory = std::string();
		state.outputDirectory = m_inputs.defaultOutputDirectory();
		state.externalDirectory = m_inputs.defaultExternalDirectory();
		state.distributionDirectory = m_inputs.defaultDistributionDirectory();
		// state.signingIdentity = std::string();
		state.osTargetName = m_inputs.getDefaultOsTargetName();
		state.osTargetVersion = m_inputs.getDefaultOsTargetVersion();
		// state.runTarget = std::string();

		if (!parseGlobalSettingsJson(state))
			return false;

		if (!parseLocalSettingsJson(state))
			return false;
	}

	tools.setSigningIdentity(m_inputs.signingIdentity());

	// If no toolchain was found in inputs or settings, use the default
	m_inputs.detectToolchainPreference();

	m_filename = m_inputs.inputFile();
	m_inputs.clearWorkingDirectory(m_filename);

	if (!Commands::pathExists(m_filename))
	{
		Diagnostic::error("Build file '{}' was not found.", m_filename);
		return false;
	}

	if (!m_chaletJson.load(m_filename))
		return false;

	if (!cache.initialize(m_inputs))
		return false;

	Output::setShowCommandOverride(false);

	if (!route.isConfigure())
	{
		Timer timer;
		Diagnostic::infoEllipsis("Reading Build File [{}]", m_filename);

		if (!parseChaletJson())
			return false;

		if (!createCache())
			return false;

		if (!validateAncillaryTools())
			return false;

		if (!validate())
			return false;

		Diagnostic::printDone(timer.asString());
	}
	else
	{
		if (!parseChaletJson())
			return false;

		// if (!externalDependencies.empty())
		{
			if (!createCache())
				return false;

			if (!validate())
				return false;
		}
	}

	Output::setShowCommandOverride(true);

	if (!route.isClean())
	{
		if (!runDependencyManager())
			return false;
	}

	return true;
}

/*****************************************************************************/
bool CentralState::initializeForList()
{
	const auto& route = m_inputs.route();
	chalet_assert(route.isQuery(), "");
	if (!route.isQuery())
		return false;

	UNUSED(cache.initializeSettings(m_inputs));

	m_filename = m_inputs.inputFile();
	if (m_filename.empty())
		m_filename = m_inputs.defaultInputFile();

	m_inputs.clearWorkingDirectory(m_filename);

	if (!Commands::pathExists(m_filename))
		return true;

	UNUSED(m_chaletJson.load(m_filename));
	Diagnostic::clearErrors();

	return true;
}

/*****************************************************************************/
bool CentralState::validate()
{
	if (!validateOsTarget())
		return false;

	if (!validateConfigurations())
		return false;

	if (!validateExternalDependencies())
		return false;

	return true;
}

/*****************************************************************************/
bool CentralState::createCache()
{
	cache.file().checkIfAppVersionChanged(m_inputs.appPath());
	cache.file().checkIfThemeChanged();

	if (!cache.createCacheFolder(CacheType::Local))
	{
		Diagnostic::error("There was an error creating the build cache.");
		return false;
	}

	return true;
}

/*****************************************************************************/
void CentralState::saveCaches()
{
	cache.saveSettings(SettingsType::Global);

	if (cache.settingsCreated())
	{
		cache.saveSettings(SettingsType::Local);

		cache.removeStaleProjectCaches();
		cache.saveProjectCache(m_inputs);
	}
}

/*****************************************************************************/
bool CentralState::runDependencyManager()
{
	DependencyManager depMgr(*this);
	if (!depMgr.run())
	{
		Diagnostic::error("There was a problem fetching one or more dependencies.");
		return false;
	}

	return true;
}

/*****************************************************************************/
void CentralState::setRunArgumentMap(Dictionary<std::string>&& inMap)
{
	m_runArgumentMap = std::move(inMap);
}

/*****************************************************************************/
const std::optional<StringList>& CentralState::getRunTargetArguments()
{
	const auto& runTarget = m_inputs.runTarget();
	if (!runTarget.empty())
	{
		if (m_runArgumentMap.find(runTarget) != m_runArgumentMap.end())
		{
			m_inputs.setRunArguments(std::move(m_runArgumentMap.at(runTarget)));
		}
	}
	m_runArgumentMap.clear();

	return m_inputs.runArguments();
}

/*****************************************************************************/
bool CentralState::validateOsTarget()
{
#if defined(CHALET_MACOS)
	auto& osTargetName = m_inputs.osTargetName();
	if (osTargetName.empty())
	{
		Diagnostic::error("Error in configuration: expected an os target, but it was blank.");
		return false;
	}

	auto allowedSdkTargets = CompilerCxxAppleClang::getAllowedSDKTargets();
	if (!String::equals(allowedSdkTargets, osTargetName))
	{
		Diagnostic::error("Error in configuration: found an invalid os target value of '{}'", osTargetName);
		return false;
	}
	auto& osTargetVersion = m_inputs.osTargetVersion();
	if (osTargetVersion.empty())
	{
		Diagnostic::error("Error in configuration: expected an os target version, but it was blank.");
		return false;
	}
	Version version;
	if (!version.setFromString(osTargetVersion))
	{
		Diagnostic::error("Error in configuration: found an invalid os target version of '{}'", osTargetVersion);
		return false;
	}
#endif

	return true;
}

/*****************************************************************************/
bool CentralState::validateConfigurations()
{
	// Unrestricted for now

	for (auto& [name, config] : m_buildConfigurations)
	{
		const bool enableProfiling = config.enableProfiling();
		const bool debugSymbols = config.debugSymbols();
		const bool lto = config.interproceduralOptimization();

		if (lto && (enableProfiling || debugSymbols))
		{
			Diagnostic::error("Error in custom configuration '{}': Enabling 'interproceduralOptimization' with 'debugSymbols' or 'enableProfiling' would cause unintended consequences. Interprocedural optimizations should only be enabled with with release builds.", m_inputs.workingDirectory());
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool CentralState::validateExternalDependencies()
{
	for (auto& dependency : externalDependencies)
	{
		if (!dependency->initialize())
		{
			Diagnostic::error("Error initializing the '{}' dependency.", dependency->name());
			return false;
		}

		// Note: dependencies are validated in the DependencyManager at the time they are run,
		// because paths might not exist yet (ie. a script)
		//
		// if (!dependency->validate())
		// {
		// 	Diagnostic::error("Error validating the '{}' dependency.", dependency->name());
		// 	return false;
		// }
	}

	return true;
}

/*****************************************************************************/
bool CentralState::validateAncillaryTools()
{
	if (!tools.validate(m_inputs.homeDirectory()))
	{
		Diagnostic::error("Error validating ancillary tools.");
		return false;
	}

	return true;
}

/*****************************************************************************/
const CommandLineInputs& CentralState::inputs() const noexcept
{
	return m_inputs;
}

/*****************************************************************************/
JsonFile& CentralState::chaletJson() noexcept
{
	return m_chaletJson;
}

/*****************************************************************************/
const JsonFile& CentralState::chaletJson() const noexcept
{
	return m_chaletJson;
}

/*****************************************************************************/
const std::string& CentralState::filename() const noexcept
{
	return m_chaletJson.filename();
}

/*****************************************************************************/
const BuildConfigurationMap& CentralState::buildConfigurations() const noexcept
{
	return m_buildConfigurations;
}

/*****************************************************************************/
bool CentralState::parseEnvFile()
{
	m_inputs.resolveEnvFile();

	DotEnvFileParser envParser(m_inputs);
	return envParser.readVariablesFromInputs();
}
/*****************************************************************************/
bool CentralState::parseGlobalSettingsJson(IntermediateSettingsState& outState)
{
	auto& settingsFile = cache.getSettings(SettingsType::Global);
	GlobalSettingsJsonParser parser(*this, settingsFile);
	return parser.serialize(outState);
}

/*****************************************************************************/
bool CentralState::parseLocalSettingsJson(const IntermediateSettingsState& inState)
{
	auto& settingsFile = cache.getSettings(SettingsType::Local);
	SettingsJsonParser parser(m_inputs, *this, settingsFile);
	return parser.serialize(inState);
}

/*****************************************************************************/
bool CentralState::parseChaletJson()
{
	CentralChaletJsonParser parser(*this);
	return parser.serialize();
}

/*****************************************************************************/
bool CentralState::makeDefaultBuildConfigurations()
{
	m_buildConfigurations.clear();

	auto buildConfigurations = BuildConfiguration::getDefaultBuildConfigurationNames();
	for (auto& name : buildConfigurations)
	{
		BuildConfiguration config;
		if (!BuildConfiguration::makeDefaultConfiguration(config, name))
		{
			Diagnostic::error("{}: Error creating the default build configurations.", m_filename);
			return false;
		}

		m_buildConfigurations.emplace(name, std::move(config));
	}

	return true;
}

/*****************************************************************************/
void CentralState::addBuildConfiguration(const std::string& inName, BuildConfiguration&& inConfig)
{
	if (m_buildConfigurations.find(inName) != m_buildConfigurations.end())
	{
		m_buildConfigurations[inName] = std::move(inConfig);
	}
	else
	{
		m_buildConfigurations.emplace(inName, std::move(inConfig));
	}
}

/*****************************************************************************/
bool CentralState::shouldPerformUpdateCheck() const
{
	return m_shouldPerformUpdateCheck;
}

/*****************************************************************************/
void CentralState::shouldCheckForUpdate(const time_t inLastUpdate, const time_t inCurrent)
{
	time_t difference = inCurrent - inLastUpdate;

	// LOG("lastUpdateCheck:", inLastUpdate);
	// LOG("currentTime:", inCurrent);
	// LOG("difference:", difference);

	time_t checkDuration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(24)).count();
	// time_t checkDuration = 5;
	// LOG("checkDuration:", checkDuration);

	m_shouldPerformUpdateCheck = difference < 0 || difference >= checkDuration;
}

/*****************************************************************************/
bool CentralState::replaceVariablesInString(std::string& outString, const IExternalDependency* inTarget, const bool inCheckHome, const std::function<std::string(std::string)>& onFail) const
{
	if (outString.empty())
		return true;

	if (inCheckHome)
	{
		const auto& homeDirectory = m_inputs.homeDirectory();
		Environment::replaceCommonVariables(outString, homeDirectory);
	}

	if (String::contains("${", outString))
	{
		if (!RegexPatterns::matchAndReplacePathVariables(outString, [&](std::string match, bool& required) {
				if (String::equals("cwd", match))
					return m_inputs.workingDirectory();

				// if (String::equals("architecture", match))
				// 	return info.targetArchitectureString();

				// if (String::equals("targetTriple", match))
				// 	return info.targetArchitectureTriple();

				// if (String::equals("configuration", match))
				// 	return configuration.name();

				if (String::equals("home", match))
					return m_inputs.homeDirectory();

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
					return Environment::getAsString(match.c_str());
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

					if (String::equals(inTarget->name(), match))
					{
						Diagnostic::error("{}: External dependency '{}' has references itself.", m_inputs.inputFile(), inTarget->name());
					}
					else
					{
						for (auto& dep : this->externalDependencies)
						{
							if (String::equals(dep->name(), inTarget->name()))
								break;

							if (String::equals(dep->name(), match))
							{
								if (dep->isGit())
								{
									return fmt::format("{}/{}", m_inputs.externalDirectory(), dep->name());
								}
								else if (dep->isLocal())
								{
									const auto& localDep = static_cast<const LocalDependency&>(*dep);
									return localDep.path();
								}
							}
						}
					}

					Diagnostic::error("{}: External dependency '{}' does not exist or is required before it's declared.", m_inputs.inputFile(), match);
					return std::string();
				}

				if (onFail != nullptr)
					return onFail(std::move(match));

				return std::string();
			}))
		{
			const auto& name = inTarget != nullptr ? inTarget->name() : std::string();
			Diagnostic::error("{}: External dependency '{}' has an unsupported variable in: {}", m_inputs.inputFile(), name, outString);
			return false;
		}
	}

	return true;
}

}
