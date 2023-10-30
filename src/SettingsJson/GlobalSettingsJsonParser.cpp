/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/GlobalSettingsJsonParser.hpp"

#include "Core/CommandLineInputs.hpp"
#include "SettingsJson/IntermediateSettingsState.hpp"
#include "State/CentralState.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
GlobalSettingsJsonParser::GlobalSettingsJsonParser(CentralState& inCentralState, JsonFile& inJsonFile) :
	m_centralState(inCentralState),
	m_jsonFile(inJsonFile)
{
	UNUSED(m_centralState, m_jsonFile);
}

/*****************************************************************************/
bool GlobalSettingsJsonParser::serialize(IntermediateSettingsState& outState)
{
	if (!makeCache(outState))
		return false;

	if (!serializeFromJsonRoot(m_jsonFile.json, outState))
	{
		Diagnostic::error("There was an error parsing {}", m_jsonFile.filename());
		return false;
	}

	return true;
}

/*****************************************************************************/
bool GlobalSettingsJsonParser::makeCache(const IntermediateSettingsState& inState)
{
	// Create the json cache
	m_jsonFile.makeNode(Keys::Options, JsonDataType::object);
	m_jsonFile.makeNode(Keys::Toolchains, JsonDataType::object);
	m_jsonFile.makeNode(Keys::Tools, JsonDataType::object);

#if defined(CHALET_MACOS)
	m_jsonFile.makeNode(Keys::AppleSdks, JsonDataType::object);
#endif

	initializeTheme();

	Json& buildOptions = m_jsonFile.json.at(Keys::Options);

	{
		// pre 6.0.0
		const std::string kRunTarget{ "runTarget" };
		if (buildOptions.contains(kRunTarget))
		{
			buildOptions.erase(kRunTarget);
			m_jsonFile.setDirty(true);
		}
	}

	auto assignSettingBool = [this, &buildOptions](const char* inKey, const bool inDefault) {
		return m_jsonFile.assignNodeIfEmpty<bool>(buildOptions, inKey, inDefault);
	};

	auto assignSettingUint = [this, &buildOptions](const char* inKey, const uint inDefault) {
		return m_jsonFile.assignNodeIfEmpty<uint>(buildOptions, inKey, inDefault);
	};

	auto assignSettingString = [this, &buildOptions](const char* inKey, const std::string& inDefault) {
		return m_jsonFile.assignNodeIfEmpty<std::string>(buildOptions, inKey, inDefault);
	};

	assignSettingBool(Keys::OptionsDumpAssembly, inState.dumpAssembly);
	assignSettingBool(Keys::OptionsShowCommands, inState.showCommands);
	assignSettingBool(Keys::OptionsBenchmark, inState.benchmark);
	assignSettingBool(Keys::OptionsLaunchProfiler, inState.launchProfiler);
	assignSettingBool(Keys::OptionsKeepGoing, inState.keepGoing);
	assignSettingBool(Keys::OptionsGenerateCompileCommands, inState.generateCompileCommands);
	assignSettingBool(Keys::OptionsOnlyRequired, inState.onlyRequired);
	assignSettingUint(Keys::OptionsMaxJobs, inState.maxJobs);
	assignSettingString(Keys::OptionsBuildConfiguration, inState.buildConfiguration);
	assignSettingString(Keys::OptionsToolchain, inState.toolchainPreference);
	assignSettingString(Keys::OptionsArchitecture, inState.architecturePreference);
	assignSettingString(Keys::OptionsInputFile, inState.inputFile);
	assignSettingString(Keys::OptionsEnvFile, inState.envFile);
	assignSettingString(Keys::OptionsOutputDirectory, inState.outputDirectory);
	assignSettingString(Keys::OptionsExternalDirectory, inState.externalDirectory);
	assignSettingString(Keys::OptionsDistributionDirectory, inState.distributionDirectory);
	assignSettingString(Keys::OptionsOsTargetName, inState.osTargetName);
	assignSettingString(Keys::OptionsOsTargetVersion, inState.osTargetVersion);
	assignSettingString(Keys::OptionsSigningIdentity, inState.signingIdentity);
	assignSettingString(Keys::OptionsLastTarget, inState.lastTarget);

	chalet_assert(inState.rootDirectory.empty(), "Root directory should never be set globally");
	buildOptions[Keys::OptionsRootDirectory] = inState.rootDirectory;

	if (!buildOptions.contains(Keys::OptionsRunArguments) || !buildOptions[Keys::OptionsRunArguments].is_object())
	{
		buildOptions[Keys::OptionsRunArguments] = Json::object();
	}

	return true;
}

/*****************************************************************************/
void GlobalSettingsJsonParser::initializeTheme()
{
	if (!m_jsonFile.json.contains(Keys::Theme))
		m_jsonFile.makeNode(Keys::Theme, JsonDataType::string);

	Json& themeJson = m_jsonFile.json.at(Keys::Theme);
	if (!themeJson.is_string() && !themeJson.is_object())
		m_jsonFile.makeNode(Keys::Theme, JsonDataType::string);

	if (themeJson.is_string())
	{
		auto preset = themeJson.get<std::string>();
		if (!ColorTheme::isValidPreset(preset))
		{
			m_jsonFile.json[Keys::Theme] = ColorTheme::getDefaultPresetName();
		}
	}
	else if (themeJson.is_object())
	{
		const auto& theme = Output::theme();
		auto makeThemeKeyValueFromTheme = [&themeJson, &theme](const std::string& inKey) {
			if (!themeJson.contains(inKey) || !themeJson[inKey].is_string())
			{
				themeJson[inKey] = theme.getAsString(inKey);
			}
		};

		for (const auto& key : ColorTheme::getKeys())
		{
			makeThemeKeyValueFromTheme(key);
		}
	}
}

/*****************************************************************************/
bool GlobalSettingsJsonParser::serializeFromJsonRoot(Json& inJson, IntermediateSettingsState& outState)
{
	if (!inJson.is_object())
	{
		Diagnostic::error("{}: Json root must be an object.", m_jsonFile.filename());
		return false;
	}

	if (!parseSettings(inJson, outState))
		return false;

	if (!parseToolchains(inJson, outState))
		return false;

	if (!parseAncillaryTools(inJson, outState))
		return false;

#if defined(CHALET_MACOS)
	if (!parseApplePlatformSdks(inJson, outState))
		return false;
#endif

	if (!parseLastUpdate(inJson))
		return false;

	return true;
}

/*****************************************************************************/
bool GlobalSettingsJsonParser::parseSettings(const Json& inNode, IntermediateSettingsState& outState)
{
	if (!inNode.contains(Keys::Options))
		return true;

	const Json& buildOptions = inNode.at(Keys::Options);
	if (!buildOptions.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), Keys::Options);
		return false;
	}

	for (const auto& [key, value] : buildOptions.items())
	{
		if (value.is_string())
		{
			if (String::equals(Keys::OptionsBuildConfiguration, key))
				outState.buildConfiguration = value.get<std::string>();
			else if (String::equals(Keys::OptionsToolchain, key))
				outState.toolchainPreference = value.get<std::string>();
			else if (String::equals(Keys::OptionsArchitecture, key))
				outState.architecturePreference = value.get<std::string>();
			else if (String::equals(Keys::OptionsSigningIdentity, key))
				outState.signingIdentity = value.get<std::string>();
			else if (String::equals(Keys::OptionsOsTargetName, key))
				outState.osTargetName = value.get<std::string>();
			else if (String::equals(Keys::OptionsOsTargetVersion, key))
				outState.osTargetVersion = value.get<std::string>();
			else if (String::equals(Keys::OptionsLastTarget, key))
				outState.lastTarget = value.get<std::string>();
			else if (String::equals(Keys::OptionsInputFile, key))
				outState.inputFile = value.get<std::string>();
			else if (String::equals(Keys::OptionsEnvFile, key))
				outState.envFile = value.get<std::string>();
			else if (String::equals(Keys::OptionsRootDirectory, key))
				outState.rootDirectory = value.get<std::string>();
			else if (String::equals(Keys::OptionsOutputDirectory, key))
				outState.outputDirectory = value.get<std::string>();
			else if (String::equals(Keys::OptionsExternalDirectory, key))
				outState.externalDirectory = value.get<std::string>();
			else if (String::equals(Keys::OptionsDistributionDirectory, key))
				outState.distributionDirectory = value.get<std::string>();
		}
		else if (value.is_boolean())
		{
			if (String::equals(Keys::OptionsShowCommands, key))
				outState.showCommands = value.get<bool>();
			else if (String::equals(Keys::OptionsDumpAssembly, key))
				outState.dumpAssembly = value.get<bool>();
			else if (String::equals(Keys::OptionsBenchmark, key))
				outState.benchmark = value.get<bool>();
			else if (String::equals(Keys::OptionsLaunchProfiler, key))
				outState.launchProfiler = value.get<bool>();
			else if (String::equals(Keys::OptionsKeepGoing, key))
				outState.keepGoing = value.get<bool>();
			else if (String::equals(Keys::OptionsGenerateCompileCommands, key))
				outState.generateCompileCommands = value.get<bool>();
			else if (String::equals(Keys::OptionsOnlyRequired, key))
				outState.onlyRequired = value.get<bool>();
		}
		else if (value.is_number())
		{
			if (String::equals(Keys::OptionsMaxJobs, key))
				outState.maxJobs = static_cast<uint>(value.get<int>());
		}
	}

	return true;
}

/*****************************************************************************/
bool GlobalSettingsJsonParser::parseToolchains(const Json& inNode, IntermediateSettingsState& outState)
{
	if (!inNode.contains(Keys::Toolchains))
		return true;

	const Json& toolchains = inNode.at(Keys::Toolchains);
	if (!toolchains.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), Keys::Toolchains);
		return false;
	}

	outState.toolchains = toolchains;

	return true;
}

/*****************************************************************************/
bool GlobalSettingsJsonParser::parseAncillaryTools(const Json& inNode, IntermediateSettingsState& outState)
{
	if (!inNode.contains(Keys::Tools))
		return true;

	const Json& tools = inNode.at(Keys::Tools);
	if (!tools.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), Keys::Tools);
		return false;
	}

	outState.tools = tools;

	return true;
}

/*****************************************************************************/
#if defined(CHALET_MACOS)
bool GlobalSettingsJsonParser::parseApplePlatformSdks(const Json& inNode, IntermediateSettingsState& outState)
{
	if (!inNode.contains(Keys::AppleSdks))
		return true;

	const Json& sdks = inNode.at(Keys::AppleSdks);
	if (!sdks.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), Keys::AppleSdks);
		return false;
	}

	outState.appleSdks = sdks;

	return true;
}
#endif

/*****************************************************************************/
bool GlobalSettingsJsonParser::parseLastUpdate(Json& outNode)
{
	time_t lastUpdateCheck = 0;
	time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	if (outNode.contains(Keys::LastUpdateCheck))
	{
		if (outNode.at(Keys::LastUpdateCheck).is_number_unsigned())
		{
			lastUpdateCheck = m_jsonFile.json.at(Keys::LastUpdateCheck).get<time_t>();
			m_centralState.shouldCheckForUpdate(lastUpdateCheck, currentTime);
		}
	}

	bool shouldUpdate = m_centralState.shouldPerformUpdateCheck();
	outNode[Keys::LastUpdateCheck] = shouldUpdate ? currentTime : lastUpdateCheck;
	if (shouldUpdate)
	{
		m_jsonFile.setDirty(true);
	}

	return true;
}

}
