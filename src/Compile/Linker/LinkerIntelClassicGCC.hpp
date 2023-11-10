/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Linker/LinkerGCC.hpp"

namespace chalet
{
struct LinkerIntelClassicGCC final : public LinkerGCC
{
	explicit LinkerIntelClassicGCC(const BuildState& inState, const SourceTarget& inProject);
};
}
