/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SETTINGS_ACTION_HPP
#define CHALET_SETTINGS_ACTION_HPP

#include "Router/RouteType.hpp"

namespace chalet
{
enum class SettingsAction : std::underlying_type_t<RouteType>
{
	Get = static_cast<std::underlying_type_t<RouteType>>(RouteType::SettingsGet),
	Set = static_cast<std::underlying_type_t<RouteType>>(RouteType::SettingsSet),
	Unset = static_cast<std::underlying_type_t<RouteType>>(RouteType::SettingsUnset),
	QueryKeys = static_cast<std::underlying_type_t<RouteType>>(RouteType::SettingsGetKeys),
};
}

#endif // CHALET_SETTINGS_ACTION_HPP
