/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "GlobalConfigJson/GlobalConfigJsonParser.hpp"

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

}
