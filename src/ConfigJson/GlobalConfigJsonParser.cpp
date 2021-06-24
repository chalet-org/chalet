/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "ConfigJson/GlobalConfigJsonParser.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/GlobalConfigState.hpp"
#include "State/StatePrototype.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
GlobalConfigJsonParser::GlobalConfigJsonParser(const CommandLineInputs& inInputs, StatePrototype& inPrototype, JsonFile& inJsonFile) :
	m_inputs(inInputs),
	m_prototype(inPrototype),
	m_jsonFile(inJsonFile)
{
	UNUSED(m_inputs, m_prototype, m_jsonFile);
}

/*****************************************************************************/
bool GlobalConfigJsonParser::serialize(GlobalConfigState& outState)
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
bool GlobalConfigJsonParser::makeCache(GlobalConfigState& outState)
{
	// Create the json cache
	m_jsonFile.makeNode(kKeySettings, JsonDataType::object);
	m_jsonFile.makeNode(kKeyToolchains, JsonDataType::object);
	m_jsonFile.makeNode(kKeyTools, JsonDataType::object);

#if defined(CHALET_MACOS)
	m_jsonFile.makeNode(kKeyApplePlatformSdks, JsonDataType::object);
#endif

	Json& settings = m_jsonFile.json[kKeySettings];

	if (!settings.contains(kKeyDumpAssembly) || !settings[kKeyDumpAssembly].is_boolean())
	{
		settings[kKeyDumpAssembly] = outState.dumpAssembly;
		m_jsonFile.setDirty(true);
	}

	if (!settings.contains(kKeyMaxJobs) || !settings[kKeyMaxJobs].is_number_integer())
	{
		outState.maxJobs = m_prototype.environment.processorCount();
		settings[kKeyMaxJobs] = outState.maxJobs;
		m_jsonFile.setDirty(true);
	}

	if (!settings.contains(kKeyShowCommands) || !settings[kKeyShowCommands].is_boolean())
	{
		settings[kKeyShowCommands] = outState.showCommands;
		m_jsonFile.setDirty(true);
	}

	if (!settings.contains(kKeyLastToolchain) || !settings[kKeyLastToolchain].is_string())
	{
		m_inputs.detectToolchainPreference();
		outState.toolchainPreference = m_inputs.toolchainPreferenceRaw();
		settings[kKeyLastToolchain] = outState.toolchainPreference;
		m_jsonFile.setDirty(true);
	}

	if (!settings.contains(kKeyMacosSigningIdentity) || !settings[kKeyMacosSigningIdentity].is_string())
	{
		outState.macosSigningIdentity = std::string();
		settings[kKeyMacosSigningIdentity] = outState.macosSigningIdentity;
		m_jsonFile.setDirty(true);
	}

	return true;
}

/*****************************************************************************/
bool GlobalConfigJsonParser::serializeFromJsonRoot(Json& inJson, GlobalConfigState& outState)
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
bool GlobalConfigJsonParser::parseSettings(const Json& inNode, GlobalConfigState& outState)
{
	if (!inNode.contains(kKeySettings))
		return true;

	const Json& settings = inNode.at(kKeySettings);
	if (!settings.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeySettings);
		return false;
	}

	if (bool val = false; m_jsonFile.assignFromKey(val, settings, kKeyShowCommands))
		outState.showCommands = val;

	if (bool val = false; m_jsonFile.assignFromKey(val, settings, kKeyDumpAssembly))
		outState.dumpAssembly = val;

	if (ushort val = 0; m_jsonFile.assignFromKey(val, settings, kKeyMaxJobs))
		outState.maxJobs = val;

	if (std::string val; m_jsonFile.assignFromKey(val, settings, kKeyLastToolchain))
		outState.toolchainPreference = std::move(val);

	if (std::string val; m_jsonFile.assignFromKey(val, settings, kKeyMacosSigningIdentity))
		outState.macosSigningIdentity = std::move(val);

	return true;
}

/*****************************************************************************/
bool GlobalConfigJsonParser::parseToolchains(const Json& inNode, GlobalConfigState& outState)
{
	if (!inNode.contains(kKeyToolchains))
		return true;

	const Json& settings = inNode.at(kKeyToolchains);
	if (!settings.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeyToolchains);
		return false;
	}

	outState.toolchains = settings;

	return true;
}

/*****************************************************************************/
bool GlobalConfigJsonParser::parseAncillaryTools(const Json& inNode, GlobalConfigState& outState)
{
	if (!inNode.contains(kKeyTools))
		return true;

	const Json& settings = inNode.at(kKeyTools);
	if (!settings.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeyTools);
		return false;
	}

	outState.ancillaryTools = settings;

	return true;
}

/*****************************************************************************/
bool GlobalConfigJsonParser::parseApplePlatformSdks(const Json& inNode, GlobalConfigState& outState)
{
	if (!inNode.contains(kKeyApplePlatformSdks))
		return true;

	const Json& settings = inNode.at(kKeyApplePlatformSdks);
	if (!settings.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeyApplePlatformSdks);
		return false;
	}

	outState.applePlatformSdks = settings;

	return true;
}

}
