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
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
StatePrototype::StatePrototype(CommandLineInputs& inInputs, std::string inFilename) :
	cache(inInputs),
	tools(inInputs),
	m_inputs(inInputs),
	m_filename(std::move(inFilename)),
	m_buildJson(std::make_unique<JsonFile>(m_filename))
{
}

/*****************************************************************************/
bool StatePrototype::initialize()
{
	m_inputs.clearWorkingDirectory(m_filename);

	if (!cache.initialize())
		return false;

	// if (!environment.initialize())
	// 	return false;

	if (!parseGlobalSettingsJson())
		return false;

	// Note: existence of m_filename is checked by Router (before the cache is made)
	if (!parseLocalSettingsJson())
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

			if (!validateDependencies())
				return false;
		}
	}

	Output::setShowCommandOverride(true);

	if (!runDependencyManager())
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
	cache.file().save();
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
	auto& bundleDirectory = m_inputs.bundleDirectory();

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

			if (!bundleDirectory.empty())
			{
				auto& outDir = bundle.outDir();
				bundle.setOutDir(fmt::format("{}/{}", bundleDirectory, outDir));
			}

			for (auto& projectName : bundle.projects())
			{
				auto res = locations.find(projectName);
				if (res != locations.end())
				{
					if (res->second == bundle.outDir())
					{
						Diagnostic::error("Project '{}' has duplicate bundle destination of '{}' defined in bundle: {}", projectName, bundle.outDir(), bundle.name());
						result = false;
					}
					else
					{
						locations.emplace(projectName, bundle.outDir());
					}
				}
				else
				{
					locations.emplace(projectName, bundle.outDir());
				}
			}
		}
	}

	return result;
}

/*****************************************************************************/
bool StatePrototype::validateDependencies()
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

	if (!validateDependencies())
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
const StringList& StatePrototype::requiredArchitectures() const noexcept
{
	return m_requiredArchitectures;
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
		outConfig.setOptimizations("3");
		outConfig.setLinkTimeOptimization(true);
		outConfig.setStripSymbols(true);
	}
	else if (String::equals("Debug", inName))
	{
		outConfig.setOptimizations("0");
		outConfig.setDebugSymbols(true);
	}
	// these two are the same as cmake
	else if (String::equals("RelWithDebInfo", inName))
	{
		outConfig.setOptimizations("2");
		outConfig.setDebugSymbols(true);
		outConfig.setLinkTimeOptimization(true);
	}
	else if (String::equals("MinSizeRel", inName))
	{
		outConfig.setOptimizations("size");
		// outConfig.setLinkTimeOptimization(true);
		outConfig.setStripSymbols(true);
	}
	else if (String::equals("Profile", inName))
	{
		outConfig.setOptimizations("0");
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
void StatePrototype::addRequiredArchitecture(std::string inArch)
{
	List::addIfDoesNotExist(m_allowedBuildConfigurations, std::move(inArch));
}
}
