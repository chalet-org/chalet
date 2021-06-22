/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SETTINGS_MANAGER_HPP
#define CHALET_SETTINGS_MANAGER_HPP

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
	std::string m_key;
	std::string m_value;

	SettingsAction m_action = SettingsAction::Get;
	SettingsType m_type = SettingsType::None;
};
}

#endif // CHALET_SETTINGS_MANAGER_HPP
