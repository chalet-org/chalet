/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SETTINGS_MANAGER_HPP
#define CHALET_SETTINGS_MANAGER_HPP

#include "Cache/WorkspaceCache.hpp"
#include "Settings/SettingsAction.hpp"
#include "Settings/SettingsType.hpp"

namespace chalet
{
struct CommandLineInputs;

struct SettingsManager
{
	explicit SettingsManager(const CommandLineInputs& inInputs, const SettingsAction inAction);

	bool run();

private:
	bool runSettingsGet(Json& node);
	bool runSettingsSet(Json& node);
	bool runSettingsUnset(Json& node);

	bool findRequestedNodeWithFailure(Json& inNode, Json*& outNode);
	bool findRequestedNode(Json& inNode, std::string& outLastKey, Json*& outNode);

	bool makeSetting(Json& inNode, Json*& outNode);
	bool getArrayKeyWithIndex(std::string& inKey, std::string& outRawKey, std::string& outIndex);

	JsonFile& getSettings();

	const CommandLineInputs& m_inputs;

	WorkspaceCache m_cache;

	std::string m_key;
	std::string m_value;

	SettingsAction m_action = SettingsAction::Get;
	SettingsType m_type = SettingsType::None;
};
}

#endif // CHALET_SETTINGS_MANAGER_HPP
