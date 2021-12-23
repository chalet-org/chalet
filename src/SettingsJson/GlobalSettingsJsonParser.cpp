/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/GlobalSettingsJsonParser.hpp"

#include <thread>

#include "Core/CommandLineInputs.hpp"
#include "SettingsJson/GlobalSettingsState.hpp"
#include "State/StatePrototype.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
GlobalSettingsJsonParser::GlobalSettingsJsonParser(StatePrototype& inPrototype, JsonFile& inJsonFile) :
	m_prototype(inPrototype),
	m_jsonFile(inJsonFile)
{
	UNUSED(m_prototype, m_jsonFile);
}

/*****************************************************************************/
bool GlobalSettingsJsonParser::serialize(GlobalSettingsState& outState)
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
bool GlobalSettingsJsonParser::makeCache(GlobalSettingsState& outState)
{
	// Create the json cache
	m_jsonFile.makeNode(Keys::Options, JsonDataType::object);
	m_jsonFile.makeNode(Keys::Toolchains, JsonDataType::object);
	m_jsonFile.makeNode(Keys::Tools, JsonDataType::object);

#if defined(CHALET_MACOS)
	m_jsonFile.makeNode(Keys::AppleSdks, JsonDataType::object);
#endif

	initializeTheme();

	Json& buildSettings = m_jsonFile.json.at(Keys::Options);

	auto assignSettingsBool = [&](const char* inKey, const bool inDefault, const std::function<std::optional<bool>()>& onGetValue) {
		m_jsonFile.assignNodeIfEmpty<bool>(buildSettings, inKey, [&inDefault]() {
			return inDefault;
		});
		auto value = onGetValue();
		if (value.has_value())
		{
			buildSettings[inKey] = *value;
		}
	};

	auto assignSettingsUint = [&](const char* inKey, const uint inDefault, const std::function<std::optional<uint>()>& onGetValue) {
		m_jsonFile.assignNodeIfEmpty<uint>(buildSettings, inKey, [&inDefault]() {
			return inDefault;
		});
		auto value = onGetValue();
		if (value.has_value())
		{
			buildSettings[inKey] = *value;
		}
	};

	auto assignSettingsString = [&](const char* inKey, const std::function<std::string()>& onAssign) {
		m_jsonFile.assignNodeIfEmpty<std::string>(buildSettings, inKey, []() {
			return std::string();
		});
		std::string value = buildSettings.at(inKey).get<std::string>();
		if (value.empty())
		{
			buildSettings[inKey] = onAssign();
		}
	};

	assignSettingsBool(Keys::OptionsDumpAssembly, outState.dumpAssembly, [&]() {
		return m_prototype.inputs().dumpAssembly();
	});

	assignSettingsBool(Keys::OptionsShowCommands, outState.showCommands, [&]() {
		return m_prototype.inputs().showCommands();
	});

	assignSettingsBool(Keys::OptionsBenchmark, outState.benchmark, [&]() {
		return m_prototype.inputs().benchmark();
	});

	assignSettingsBool(Keys::OptionsLaunchProfiler, outState.launchProfiler, [&]() {
		return m_prototype.inputs().launchProfiler();
	});

	assignSettingsBool(Keys::OptionsGenerateCompileCommands, outState.generateCompileCommands, [&]() {
		return m_prototype.inputs().generateCompileCommands();
	});

	{
		uint maxJobs = outState.maxJobs == 0 ? std::thread::hardware_concurrency() : outState.maxJobs;
		assignSettingsUint(Keys::OptionsMaxJobs, maxJobs, [&]() {
			return m_prototype.inputs().maxJobs();
		});
	}

	assignSettingsString(Keys::OptionsBuildConfiguration, [&]() {
		outState.buildConfiguration = m_prototype.inputs().buildConfiguration();
		return outState.buildConfiguration;
	});

	assignSettingsString(Keys::OptionsToolchain, [&]() {
		m_prototype.inputs().detectToolchainPreference();
		outState.toolchainPreference = m_prototype.inputs().toolchainPreferenceName();
		return outState.toolchainPreference;
	});

	assignSettingsString(Keys::OptionsArchitecture, [&]() {
		outState.architecturePreference = m_prototype.inputs().architectureRaw();
		return outState.architecturePreference;
	});

	assignSettingsString(Keys::OptionsInputFile, [&]() {
		outState.inputFile = m_prototype.inputs().defaultInputFile();
		return outState.inputFile;
	});

	assignSettingsString(Keys::OptionsEnvFile, [&]() {
		outState.envFile = m_prototype.inputs().defaultEnvFile();
		return outState.envFile;
	});

	assignSettingsString(Keys::OptionsRootDirectory, [&]() {
		outState.rootDirectory = m_prototype.inputs().rootDirectory();
		return outState.rootDirectory;
	});

	assignSettingsString(Keys::OptionsOutputDirectory, [&]() {
		outState.outputDirectory = m_prototype.inputs().defaultOutputDirectory();
		return outState.outputDirectory;
	});

	assignSettingsString(Keys::OptionsExternalDirectory, [&]() {
		outState.externalDirectory = m_prototype.inputs().defaultExternalDirectory();
		return outState.externalDirectory;
	});

	assignSettingsString(Keys::OptionsDistributionDirectory, [&]() {
		outState.distributionDirectory = m_prototype.inputs().defaultDistributionDirectory();
		return outState.distributionDirectory;
	});

	assignSettingsString(Keys::OptionsSigningIdentity, [&]() {
		outState.signingIdentity = std::string();
		return outState.signingIdentity;
	});

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
			m_jsonFile.json[Keys::Theme] = ColorTheme::defaultPresetName();
		}
	}
	else if (themeJson.is_object())
	{
		const auto& theme = Output::theme();
		auto makeThemeKeyValueFromTheme = [&](const std::string& inKey) {
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
bool GlobalSettingsJsonParser::serializeFromJsonRoot(Json& inJson, GlobalSettingsState& outState)
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

	return true;
}

/*****************************************************************************/
bool GlobalSettingsJsonParser::parseSettings(const Json& inNode, GlobalSettingsState& outState)
{
	if (!inNode.contains(Keys::Options))
		return true;

	const Json& buildSettings = inNode.at(Keys::Options);
	if (!buildSettings.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), Keys::Options);
		return false;
	}

	for (const auto& [key, value] : buildSettings.items())
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
			else if (String::equals(Keys::OptionsGenerateCompileCommands, key))
				outState.generateCompileCommands = value.get<bool>();
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
bool GlobalSettingsJsonParser::parseToolchains(const Json& inNode, GlobalSettingsState& outState)
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
bool GlobalSettingsJsonParser::parseAncillaryTools(const Json& inNode, GlobalSettingsState& outState)
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
bool GlobalSettingsJsonParser::parseApplePlatformSdks(const Json& inNode, GlobalSettingsState& outState)
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

}
