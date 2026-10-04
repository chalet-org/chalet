/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Core/Router/RouteType.hpp"

namespace chalet
{
enum class SettingsAction : u16
{
	Get,
	Set,
	Unset,
	QueryKeys,
};
}
