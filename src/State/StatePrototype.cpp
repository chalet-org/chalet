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

#include "State/Distribution/BundleTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
StatePrototype::StatePrototype(CommandLineInputs& inInputs) :
	cache(inInputs),
	tools(inInputs),
	m_inputs(inInputs),
	m_buildJson(std::make_unique<JsonFile>())
{
}

/*****************************************************************************/
bool StatePrototype::initialize()
{
	if (!cache.initializeSettings())
		return false;

	// if (!environment.initialize())
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

	m_buildJson->load(m_inputs.inputFile());

	if (!cache.initialize())
		return false;

	Output::setShowCommandOverride(false);

	if (m_inputs.command() != Route::Configure)
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
		{
			BuildJsonProtoParser parser(m_inputs, *this);
			if (!parser.serializeDependenciesOnly())
				return false;
		}

		if (!externalDependencies.empty())
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
	cache.saveSettings(SettingsType::Local);
	cache.saveSettings(SettingsType::Global);

	cache.removeStaleProjectCaches();
	cache.saveProjectCache();
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

	std::unordered_map<std::string, std::string> locations;
	bool result = true;
	for (auto& target : distribution)
	{
		if (target->isDistributionBundle())
		{
			auto& bundle = static_cast<BundleTarget&>(*target);

			if (bundle.configuration().empty() && !m_releaseConfiguration.empty())
			{
				auto config = m_releaseConfiguration;
				bundle.setConfiguration(std::move(config));
			}
			List::addIfDoesNotExist(m_requiredBuildConfigurations, bundle.configuration());

			if (!distributionDirectory.empty())
			{
				auto& subDirectory = bundle.subDirectory();
				bundle.setSubDirectory(fmt::format("{}/{}", distributionDirectory, subDirectory));
			}

			for (auto& projectName : bundle.projects())
			{
				auto res = locations.find(projectName);
				if (res != locations.end())
				{
					if (res->second == bundle.subDirectory())
					{
						Diagnostic::error("Project '{}' has duplicate bundle destination of '{}' defined in bundle: {}", projectName, bundle.subDirectory(), bundle.name());
						result = false;
					}
					else
					{
						locations.emplace(projectName, bundle.subDirectory());
					}
				}
				else
				{
					locations.emplace(projectName, bundle.subDirectory());
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
	if (!tools.validate())
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
JsonFile& StatePrototype::jsonFile() noexcept
{
	return *m_buildJson;
}

/*****************************************************************************/
const std::string& StatePrototype::filename() const noexcept
{
	return m_buildJson->filename();
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

	m_allowedBuildConfigurations = {
		"Release",
		"Debug",
		"RelWithDebInfo",
		"MinSizeRel",
		"Profile",
	};

	m_releaseConfiguration = "Release";

	for (auto& name : m_allowedBuildConfigurations)
	{
		BuildConfiguration config;
		if (!getDefaultBuildConfiguration(config, name))
		{
			Diagnostic::error("{}: Error creating the default build configurations.", m_filename);
			return false;
		}

		m_buildConfigurations.emplace(std::move(name), std::move(config));
	}

	return true;
}

/*****************************************************************************/
bool StatePrototype::getDefaultBuildConfiguration(BuildConfiguration& outConfig, const std::string& inName) const
{
	if (String::equals("Release", inName))
	{
		outConfig.setOptimizationLevel("3");
		outConfig.setLinkTimeOptimization(true);
		outConfig.setStripSymbols(true);
	}
	else if (String::equals("Debug", inName))
	{
		outConfig.setOptimizationLevel("0");
		outConfig.setDebugSymbols(true);
	}
	// these two are the same as cmake
	else if (String::equals("RelWithDebInfo", inName))
	{
		outConfig.setOptimizationLevel("2");
		outConfig.setDebugSymbols(true);
		outConfig.setLinkTimeOptimization(false);
	}
	else if (String::equals("MinSizeRel", inName))
	{
		outConfig.setOptimizationLevel("size");
		// outConfig.setLinkTimeOptimization(true);
		outConfig.setStripSymbols(true);
	}
	else if (String::equals("Profile", inName))
	{
		outConfig.setOptimizationLevel("0");
		outConfig.setDebugSymbols(true);
		outConfig.setEnableProfiling(true);
	}
	else
	{
		Diagnostic::error("{}: An invalid build configuration ({}) was requested. Expected: Release, Debug, RelWithDebInfo, MinSizeRel, Profile", m_filename, inName);
		return false;
	}

	outConfig.setName(inName);

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
