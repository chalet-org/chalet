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

namespace chalet
{
/*****************************************************************************/
GlobalSettingsJsonParser::GlobalSettingsJsonParser(const CommandLineInputs& inInputs, StatePrototype& inPrototype, JsonFile& inJsonFile) :
	m_inputs(inInputs),
	m_prototype(inPrototype),
	m_jsonFile(inJsonFile)
{
	UNUSED(m_inputs, m_prototype, m_jsonFile);
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
	m_jsonFile.makeNode(kKeyOptions, JsonDataType::object);
	m_jsonFile.makeNode(kKeyToolchains, JsonDataType::object);
	m_jsonFile.makeNode(kKeyTools, JsonDataType::object);

#if defined(CHALET_MACOS)
	m_jsonFile.makeNode(kKeyAppleSdks, JsonDataType::object);
#endif

	initializeTheme();

	Json& buildSettings = m_jsonFile.json.at(kKeyOptions);

	auto assignSettingsBool = [&](const std::string& inKey, const bool inDefault, const std::function<std::optional<bool>()>& onGetValue) {
		m_jsonFile.assignNodeIfEmpty<bool>(buildSettings, inKey, [&inDefault]() {
			return inDefault;
		});
		auto value = onGetValue();
		if (value.has_value())
		{
			buildSettings[inKey] = *value;
		}
	};

	auto assignSettingsUint = [&](const std::string& inKey, const uint inDefault, const std::function<std::optional<uint>()>& onGetValue) {
		m_jsonFile.assignNodeIfEmpty<uint>(buildSettings, inKey, [&inDefault]() {
			return inDefault;
		});
		auto value = onGetValue();
		if (value.has_value())
		{
			buildSettings[inKey] = *value;
		}
	};

	auto assignSettingsString = [&](const std::string& inKey, const std::function<std::string()>& onAssign) {
		m_jsonFile.assignNodeIfEmpty<std::string>(buildSettings, inKey, []() {
			return std::string();
		});
		std::string value = buildSettings.at(inKey).get<std::string>();
		if (value.empty())
		{
			buildSettings[inKey] = onAssign();
		}
	};

	assignSettingsBool(kKeyDumpAssembly, outState.dumpAssembly, [&]() {
		return m_inputs.dumpAssembly();
	});

	assignSettingsBool(kKeyShowCommands, outState.showCommands, [&]() {
		return m_inputs.showCommands();
	});

	assignSettingsBool(kKeyBenchmark, outState.benchmark, [&]() {
		return m_inputs.benchmark();
	});

	assignSettingsBool(kKeyLaunchProfiler, outState.launchProfiler, [&]() {
		return m_inputs.launchProfiler();
	});

	assignSettingsBool(kKeyGenerateCompileCommands, outState.generateCompileCommands, [&]() {
		return m_inputs.generateCompileCommands();
	});

	{
		uint maxJobs = outState.maxJobs == 0 ? std::thread::hardware_concurrency() : outState.maxJobs;
		assignSettingsUint(kKeyMaxJobs, maxJobs, [&]() {
			return m_inputs.maxJobs();
		});
	}

	assignSettingsString(kKeyLastBuildConfiguration, [&]() {
		outState.buildConfiguration = m_inputs.buildConfiguration();
		return outState.buildConfiguration;
	});

	assignSettingsString(kKeyLastToolchain, [&]() {
		m_inputs.detectToolchainPreference();
		outState.toolchainPreference = m_inputs.toolchainPreferenceName();
		return outState.toolchainPreference;
	});

	assignSettingsString(kKeyLastArchitecture, [&]() {
		outState.architecturePreference = m_inputs.architectureRaw();
		return outState.architecturePreference;
	});

	assignSettingsString(kKeyInputFile, [&]() {
		outState.inputFile = m_inputs.defaultInputFile();
		return outState.inputFile;
	});

	assignSettingsString(kKeyEnvFile, [&]() {
		outState.envFile = m_inputs.defaultEnvFile();
		return outState.envFile;
	});

	assignSettingsString(kKeyRootDirectory, [&]() {
		outState.rootDirectory = m_inputs.rootDirectory();
		return outState.rootDirectory;
	});

	assignSettingsString(kKeyOutputDirectory, [&]() {
		outState.outputDirectory = m_inputs.defaultOutputDirectory();
		return outState.outputDirectory;
	});

	assignSettingsString(kKeyExternalDirectory, [&]() {
		outState.externalDirectory = m_inputs.defaultExternalDirectory();
		return outState.externalDirectory;
	});

	assignSettingsString(kKeyDistributionDirectory, [&]() {
		outState.distributionDirectory = m_inputs.defaultDistributionDirectory();
		return outState.distributionDirectory;
	});

	assignSettingsString(kKeySigningIdentity, [&]() {
		outState.signingIdentity = std::string();
		return outState.signingIdentity;
	});

	return true;
}

/*****************************************************************************/
void GlobalSettingsJsonParser::initializeTheme()
{
	if (!m_jsonFile.json.contains(kKeyTheme))
		m_jsonFile.makeNode(kKeyTheme, JsonDataType::string);

	Json& themeJson = m_jsonFile.json.at(kKeyTheme);
	if (!themeJson.is_string() && !themeJson.is_object())
		m_jsonFile.makeNode(kKeyTheme, JsonDataType::string);

	if (themeJson.is_string())
	{
		auto preset = themeJson.get<std::string>();
		if (!ColorTheme::isValidPreset(preset))
		{
			m_jsonFile.json[kKeyTheme] = ColorTheme::defaultPresetName();
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
	if (!inNode.contains(kKeyOptions))
		return true;

	const Json& buildSettings = inNode.at(kKeyOptions);
	if (!buildSettings.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeyOptions);
		return false;
	}

	for (const auto& [key, value] : buildSettings.items())
	{
		if (value.is_string())
		{
			if (String::equals(kKeyLastBuildConfiguration, key))
				outState.buildConfiguration = value.get<std::string>();
			else if (String::equals(kKeyLastToolchain, key))
				outState.toolchainPreference = value.get<std::string>();
			else if (String::equals(kKeyLastArchitecture, key))
				outState.architecturePreference = value.get<std::string>();
			else if (String::equals(kKeySigningIdentity, key))
				outState.signingIdentity = value.get<std::string>();
			else if (String::equals(kKeyInputFile, key))
				outState.inputFile = value.get<std::string>();
			else if (String::equals(kKeyEnvFile, key))
				outState.envFile = value.get<std::string>();
			else if (String::equals(kKeyRootDirectory, key))
				outState.rootDirectory = value.get<std::string>();
			else if (String::equals(kKeyOutputDirectory, key))
				outState.outputDirectory = value.get<std::string>();
			else if (String::equals(kKeyExternalDirectory, key))
				outState.externalDirectory = value.get<std::string>();
			else if (String::equals(kKeyDistributionDirectory, key))
				outState.distributionDirectory = value.get<std::string>();
		}
		else if (value.is_boolean())
		{
			if (String::equals(kKeyShowCommands, key))
				outState.showCommands = value.get<bool>();
			else if (String::equals(kKeyDumpAssembly, key))
				outState.dumpAssembly = value.get<bool>();
			else if (String::equals(kKeyBenchmark, key))
				outState.benchmark = value.get<bool>();
			else if (String::equals(kKeyLaunchProfiler, key))
				outState.launchProfiler = value.get<bool>();
			else if (String::equals(kKeyGenerateCompileCommands, key))
				outState.generateCompileCommands = value.get<bool>();
		}
		else if (value.is_number())
		{
			if (String::equals(kKeyMaxJobs, key))
				outState.maxJobs = static_cast<uint>(value.get<int>());
		}
	}

	return true;
}

/*****************************************************************************/
bool GlobalSettingsJsonParser::parseToolchains(const Json& inNode, GlobalSettingsState& outState)
{
	if (!inNode.contains(kKeyToolchains))
		return true;

	const Json& toolchains = inNode.at(kKeyToolchains);
	if (!toolchains.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeyToolchains);
		return false;
	}

	outState.toolchains = toolchains;

	return true;
}

/*****************************************************************************/
bool GlobalSettingsJsonParser::parseAncillaryTools(const Json& inNode, GlobalSettingsState& outState)
{
	if (!inNode.contains(kKeyTools))
		return true;

	const Json& tools = inNode.at(kKeyTools);
	if (!tools.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeyTools);
		return false;
	}

	outState.tools = tools;

	return true;
}

/*****************************************************************************/
#if defined(CHALET_MACOS)
bool GlobalSettingsJsonParser::parseApplePlatformSdks(const Json& inNode, GlobalSettingsState& outState)
{
	if (!inNode.contains(kKeyAppleSdks))
		return true;

	const Json& sdks = inNode.at(kKeyAppleSdks);
	if (!sdks.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeyAppleSdks);
		return false;
	}

	outState.appleSdks = sdks;

	return true;
}
#endif

}
