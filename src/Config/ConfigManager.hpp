/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CONFIG_MANAGER_HPP
#define CHALET_CONFIG_MANAGER_HPP

#include "Cache/WorkspaceCache.hpp"
#include "Config/ConfigAction.hpp"
#include "Config/ConfigType.hpp"

namespace chalet
{
struct CommandLineInputs;

struct ConfigManager
{
	explicit ConfigManager(const CommandLineInputs& inInputs, const ConfigAction inAction);

	bool run();

private:
	bool runSettingsGet(Json& node);
	bool runSettingsSet(Json& node);
	bool runSettingsUnset(Json& node);

	bool findRequestedNodeWithFailure(Json& inNode, Json*& outNode);
	bool findRequestedNode(Json& inNode, std::string& outLastKey, Json*& outNode);

	bool makeSetting(Json& inNode, Json*& outNode);

	JsonFile& getConfig();

	WorkspaceCache m_cache;

	std::string m_key;
	std::string m_value;

	ConfigAction m_action = ConfigAction::Get;
	ConfigType m_type = ConfigType::None;
};
}

#endif // CHALET_CONFIG_MANAGER_HPP
