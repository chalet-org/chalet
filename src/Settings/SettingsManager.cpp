/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Settings/SettingsManager.hpp"

#include "Core/CommandLineInputs.hpp"

namespace chalet
{
/*****************************************************************************/
SettingsManager::SettingsManager(const CommandLineInputs& inInputs, const SettingsAction inAction) :
	m_key(inInputs.settingsKey()),
	m_value(inInputs.settingsValue()),
	m_action(inAction),
	m_type(inInputs.settingsType())
{
}

/*****************************************************************************/
bool SettingsManager::run()
{
	auto type = m_type == SettingsType::Local ? "local" : "global";
	if (m_action == SettingsAction::Set)
		LOG("set:", type, m_key, m_value);
	else
		LOG("get:", type, m_key, m_value);

	return true;
}
}
