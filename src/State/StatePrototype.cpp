/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/StatePrototype.hpp"

#include "BuildJson/BuildJsonProtoParser.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Dependencies/DependencyManager.hpp"
#include "SettingsJson/GlobalSettingsJsonParser.hpp"
#include "SettingsJson/SettingsJsonParser.hpp"

#include "Core/DotEnvFileParser.hpp"
#include "State/Distribution/BundleTarget.hpp"
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
StatePrototype::StatePrototype(CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
}

/*****************************************************************************/
bool StatePrototype::initialize()
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

		if (!parseBuildJson())
			return false;

		if (!validateBundleDestinations())
			return false;

		if (!createCache())
			return false;

		if (!validateBuildFile())
			return false;

		Diagnostic::printDone(timer.asString());
	}
	else
	{
		if (!parseBuildJson())
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
bool StatePrototype::initializeForList()
{
	Route route = m_inputs.route();
	chalet_assert(route == Route::Query, "");
	if (route != Route::Query)
		return false;

	if (!cache.initializeSettings(m_inputs))
		return false;

	m_filename = m_inputs.inputFile();
	if (m_filename.empty())
		m_filename = m_inputs.defaultInputFile();

	m_inputs.clearWorkingDirectory(m_filename);

	if (!Commands::pathExists(m_filename))
		return true;

	if (!m_chaletJson.load(m_filename))
		return false;

	return true;
}

/*****************************************************************************/
bool StatePrototype::validate()
{
	// if (!validateConfigurations())
	// 	return false;

	if (!validateExternalDependencies())
		return false;

	return true;
}

/*****************************************************************************/
bool StatePrototype::createCache()
{
	cache.file().checkIfAppVersionChanged(m_inputs.appPath());

	if (!cache.createCacheFolder(CacheType::Local))
	{
		Diagnostic::error("There was an error creating the build cache.");
		return false;
	}

	return true;
}

/*****************************************************************************/
void StatePrototype::saveCaches()
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
bool StatePrototype::runDependencyManager()
{
	DependencyManager depMgr(m_inputs, *this);
	if (!depMgr.run())
	{
		Diagnostic::error("There was an error creating the dependencies.");
		return false;
	}

	return true;
}

/*****************************************************************************/
bool StatePrototype::validateBundleDestinations()
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
/*bool StatePrototype::validateConfigurations()
{
	// Unrestricted for now

	// for (auto& [name, config] : m_buildConfigurations)
	// {
	// 	const bool enableProfiling = config.enableProfiling();
	// 	const bool debugSymbols = config.debugSymbols();
	// 	const bool lto = config.linkTimeOptimization();

	// 	if (lto && (enableProfiling || debugSymbols))
	// 	{
	// 		Diagnostic::error("Error in custom configuration '{}': Enabling 'linkTimeOptimization' with 'debugSymbols' or 'enableProfiling' would cause unintended consequences. Link-time optimizations are typically used with release builds.", m_inputs.workingDirectory());
	// 		return false;
	// 	}
	// }
	return true;
}*/

/*****************************************************************************/
bool StatePrototype::validateExternalDependencies()
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
bool StatePrototype::validateBuildFile()
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
JsonFile& StatePrototype::chaletJson() noexcept
{
	return m_chaletJson;
}

/*****************************************************************************/
const JsonFile& StatePrototype::chaletJson() const noexcept
{
	return m_chaletJson;
}

/*****************************************************************************/
const std::string& StatePrototype::filename() const noexcept
{
	return m_chaletJson.filename();
}

/*****************************************************************************/
const BuildConfigurationMap& StatePrototype::buildConfigurations() const noexcept
{
	return m_buildConfigurations;
}

/*****************************************************************************/
const StringList& StatePrototype::requiredBuildConfigurations() const noexcept
{
	return m_requiredBuildConfigurations;
}

/*****************************************************************************/
const std::string& StatePrototype::releaseConfiguration() const noexcept
{
	return m_releaseConfiguration;
}

/*****************************************************************************/
const std::string& StatePrototype::anyConfiguration() const noexcept
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
bool StatePrototype::parseEnvFile()
{
	DotEnvFileParser envParser(m_inputs);
	return envParser.serialize();
}
/*****************************************************************************/
bool StatePrototype::parseGlobalSettingsJson()
{
	auto& settingsFile = cache.getSettings(SettingsType::Global);
	GlobalSettingsJsonParser parser(m_inputs, *this, settingsFile);
	return parser.serialize(m_globalSettingsState);
}

/*****************************************************************************/
bool StatePrototype::parseLocalSettingsJson()
{
	auto& settingsFile = cache.getSettings(SettingsType::Local);
	SettingsJsonParser parser(m_inputs, *this, settingsFile);
	return parser.serialize(m_globalSettingsState);
}

/*****************************************************************************/
bool StatePrototype::parseBuildJson()
{
	BuildJsonProtoParser parser(m_inputs, *this);
	return parser.serialize();
}

/*****************************************************************************/
bool StatePrototype::makeDefaultBuildConfigurations()
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

		m_buildConfigurations.emplace(std::move(name), std::move(config));
	}

	return true;
}

/*****************************************************************************/
void StatePrototype::addBuildConfiguration(const std::string&& inName, BuildConfiguration&& inConfig)
{
	m_buildConfigurations.emplace(std::move(inName), std::move(inConfig));
}

/*****************************************************************************/
void StatePrototype::setReleaseConfiguration(const std::string& inName)
{
	m_releaseConfiguration = inName;
}

/*****************************************************************************/
void StatePrototype::addRequiredBuildConfiguration(std::string inValue)
{
	List::addIfDoesNotExist(m_allowedBuildConfigurations, std::move(inValue));
}
}
