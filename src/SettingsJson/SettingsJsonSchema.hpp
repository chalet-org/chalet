/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"

namespace chalet
{
struct CommandLineInputs;

namespace SettingsJsonSchema
{
Json get(const CommandLineInputs& inInputs);
}
}
