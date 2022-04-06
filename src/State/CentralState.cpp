/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/CentralState.hpp"

#include "ChaletJson/CentralChaletJsonParser.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Dependencies/DependencyManager.hpp"
#include "SettingsJson/GlobalSettingsJsonParser.hpp"
#include "SettingsJson/SettingsJsonParser.hpp"

#include "Core/DotEnvFileParser.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/WindowsTerminal.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
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

	Route route = m_inputs.route();
	chalet_assert(route != Route::Query, "");

	if (!parseEnvFile())
		return false;

	if (!cache.initializeSettings(m_inputs))
		return false;

	// if (!workspace.initialize())
	// 	return false;

	if (!parseGlobalSettingsJson())
		return false;

	if (!parseLocalSettingsJson())
		return false;

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

	if (route != Route::Configure)
	{
		Timer timer;
		Diagnostic::infoEllipsis("Reading Build File [{}]", m_filename);

		if (!parseChaletJson())
			return false;

		if (!validateDistribution())
			return false;

		if (!createCache())
			return false;

		if (!validateBuildFile())
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

	if (!runDependencyManager())
		return false;

	return true;
}

/*****************************************************************************/
bool CentralState::initializeForList()
{
	Route route = m_inputs.route();
	chalet_assert(route == Route::Query, "");
	if (route != Route::Query)
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

void CentralState::getRunTargetArguments()
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
}

/*****************************************************************************/
bool CentralState::validateDistribution()
{
	auto& distributionDirectory = m_inputs.distributionDirectory();

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

			if (bundle.configuration().empty() && !m_releaseConfiguration.empty())
			{
				auto config = m_releaseConfiguration;
				bundle.setConfiguration(std::move(config));
			}
			List::addIfDoesNotExist(m_requiredBuildConfigurations, bundle.configuration());

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
		if (!dependency->validate())
		{
			Diagnostic::error("Error validating the '{}' external dependency.", dependency->name());
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool CentralState::validateBuildFile()
{
	if (!tools.validate(m_inputs.homeDirectory()))
	{
		Diagnostic::error("Error validating ancillary tools.");
		return false;
	}

	if (!validate())
		return false;

	for (auto& target : distribution)
	{
		if (!target->validate())
		{
			Diagnostic::error("Error validating the '{}' distribution target.", target->name());
			return false;
		}
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
const StringList& CentralState::requiredBuildConfigurations() const noexcept
{
	return m_requiredBuildConfigurations;
}

/*****************************************************************************/
const std::string& CentralState::releaseConfiguration() const noexcept
{
	return m_releaseConfiguration;
}

/*****************************************************************************/
const std::string& CentralState::anyConfiguration() const noexcept
{
	if (!m_requiredBuildConfigurations.empty() && !m_requiredBuildConfigurations.front().empty())
	{
		return m_requiredBuildConfigurations.front();
	}
	else
	{
		return m_releaseConfiguration;
	}
}

/*****************************************************************************/
void CentralState::replaceVariablesInPath(std::string& outPath, const std::string& inName) const
{
	if (outPath.empty())
		return;

	const auto& distributionDir = m_inputs.distributionDirectory();
	const auto& externalDir = m_inputs.externalDirectory();
	const auto& cwd = m_inputs.workingDirectory();
	const auto& homeDirectory = m_inputs.homeDirectory();
	const auto& versionString = workspace.metadata().versionString();
	const auto& version = workspace.metadata().version();

	if (!cwd.empty())
		String::replaceAll(outPath, "${cwd}", cwd);

	String::replaceAll(outPath, "${configuration}", anyConfiguration());

	if (!distributionDir.empty())
		String::replaceAll(outPath, "${distributionDir}", distributionDir);

	if (!externalDir.empty())
		String::replaceAll(outPath, "${externalDir}", externalDir);

	if (!inName.empty())
		String::replaceAll(outPath, "${name}", inName);

	if (!versionString.empty())
	{
		if (String::contains("${version", outPath))
		{
			String::replaceAll(outPath, "${version}", versionString);

			String::replaceAll(outPath, "${versionMajor}", std::to_string(version.major()));

			if (version.hasMinor())
				String::replaceAll(outPath, "${versionMinor}", std::to_string(version.minor()));
			else
				String::replaceAll(outPath, "${versionMinor}", "");

			if (version.hasPatch())
				String::replaceAll(outPath, "${versionPatch}", std::to_string(version.patch()));
			else
				String::replaceAll(outPath, "${versionPatch}", "");

			if (version.hasTweak())
				String::replaceAll(outPath, "${versionTweak}", std::to_string(version.tweak()));
			else
				String::replaceAll(outPath, "${versionTweak}", "");
		}
	}

	Environment::replaceCommonVariables(outPath, homeDirectory);
}

/*****************************************************************************/
bool CentralState::parseEnvFile()
{
	m_inputs.resolveEnvFile();

	DotEnvFileParser envParser(m_inputs);
	return envParser.readVariablesFromInputs();
}
/*****************************************************************************/
bool CentralState::parseGlobalSettingsJson()
{
	auto& settingsFile = cache.getSettings(SettingsType::Global);
	GlobalSettingsJsonParser parser(*this, settingsFile);
	return parser.serialize(m_globalSettingsState);
}

/*****************************************************************************/
bool CentralState::parseLocalSettingsJson()
{
	auto& settingsFile = cache.getSettings(SettingsType::Local);
	SettingsJsonParser parser(m_inputs, *this, settingsFile);
	return parser.serialize(m_globalSettingsState);
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

	m_allowedBuildConfigurations = BuildConfiguration::getDefaultBuildConfigurationNames();

	m_releaseConfiguration = "Release";

	for (auto& name : m_allowedBuildConfigurations)
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
void CentralState::setReleaseConfiguration(const std::string& inName)
{
	m_releaseConfiguration = inName;
}

/*****************************************************************************/
void CentralState::addRequiredBuildConfiguration(std::string inValue)
{
	List::addIfDoesNotExist(m_allowedBuildConfigurations, std::move(inValue));
}
}
