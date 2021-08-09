/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/GlobalSettingsJsonParser.hpp"

#include "Core/CommandLineInputs.hpp"
#include "SettingsJson/GlobalSettingsState.hpp"
#include "State/StatePrototype.hpp"
#include "Terminal/Output.hpp"
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
	m_jsonFile.makeNode(kKeySettings, JsonDataType::object);
	m_jsonFile.makeNode(kKeyToolchains, JsonDataType::object);
	m_jsonFile.makeNode(kKeyTools, JsonDataType::object);

#if defined(CHALET_MACOS)
	m_jsonFile.makeNode(kKeyAppleSdks, JsonDataType::object);
#endif

	m_jsonFile.makeNode(kKeyTheme, JsonDataType::object);

	ColorTheme theme = Output::theme();
	Json& themeJson = m_jsonFile.json[kKeyTheme];
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

	Json& buildSettings = m_jsonFile.json[kKeySettings];

	m_jsonFile.assignNodeIfEmpty<bool>(buildSettings, kKeyDumpAssembly, [&]() {
		return outState.dumpAssembly;
	});

	m_jsonFile.assignNodeIfEmpty<bool>(buildSettings, kKeyGenerateCompileCommands, [&]() {
		return outState.generateCompileCommands;
	});

	m_jsonFile.assignNodeIfEmpty<uint>(buildSettings, kKeyMaxJobs, [&]() {
		outState.maxJobs = m_prototype.environment.processorCount();
		return outState.maxJobs;
	});

	m_jsonFile.assignNodeIfEmpty<bool>(buildSettings, kKeyShowCommands, [&]() {
		return outState.showCommands;
	});

	m_jsonFile.assignNodeIfEmpty<std::string>(buildSettings, kKeyLastToolchain, [&]() {
		m_inputs.detectToolchainPreference();
		outState.toolchainPreference = m_inputs.toolchainPreferenceName();
		return outState.toolchainPreference;
	});

	m_jsonFile.assignNodeIfEmpty<std::string>(buildSettings, kKeyLastArchitecture, [&]() {
		outState.architecturePreference = m_inputs.architectureRaw();
		return outState.architecturePreference;
	});

	m_jsonFile.assignNodeIfEmpty<std::string>(buildSettings, kKeyInputFile, [&]() {
		outState.inputFile = m_inputs.inputFile();
		return outState.inputFile;
	});

	m_jsonFile.assignNodeIfEmpty<std::string>(buildSettings, kKeyEnvFile, [&]() {
		outState.envFile = m_inputs.envFile();
		return outState.envFile;
	});

	m_jsonFile.assignNodeIfEmpty<std::string>(buildSettings, kKeyRootDirectory, [&]() {
		outState.rootDirectory = m_inputs.rootDirectory();
		return outState.rootDirectory;
	});

	m_jsonFile.assignNodeIfEmpty<std::string>(buildSettings, kKeyOutputDirectory, [&]() {
		outState.outputDirectory = m_inputs.outputDirectory();
		return outState.rootDirectory;
	});

	m_jsonFile.assignNodeIfEmpty<std::string>(buildSettings, kKeyBundleDirectory, [&]() {
		outState.bundleDirectory = m_inputs.bundleDirectory();
		return outState.bundleDirectory;
	});

	m_jsonFile.assignNodeIfEmpty<std::string>(buildSettings, kKeySigningIdentity, [&]() {
		outState.signingIdentity = std::string();
		return outState.signingIdentity;
	});

	return true;
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
	if (!inNode.contains(kKeySettings))
		return true;

	const Json& buildSettings = inNode.at(kKeySettings);
	if (!buildSettings.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeySettings);
		return false;
	}

	if (bool val = false; m_jsonFile.assignFromKey(val, buildSettings, kKeyShowCommands))
		outState.showCommands = val;

	if (bool val = false; m_jsonFile.assignFromKey(val, buildSettings, kKeyDumpAssembly))
		outState.dumpAssembly = val;

	if (ushort val = 0; m_jsonFile.assignFromKey(val, buildSettings, kKeyMaxJobs))
		outState.maxJobs = val;

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeyLastToolchain))
		outState.toolchainPreference = std::move(val);

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeyLastArchitecture))
		outState.architecturePreference = std::move(val);

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeySigningIdentity))
		outState.signingIdentity = std::move(val);

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

}
