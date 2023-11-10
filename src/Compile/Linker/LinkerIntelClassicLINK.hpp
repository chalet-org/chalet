/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Linker/LinkerVisualStudioLINK.hpp"

namespace chalet
{
struct LinkerIntelClassicLINK : public LinkerVisualStudioLINK
{
	explicit LinkerIntelClassicLINK(const BuildState& inState, const SourceTarget& inProject);
};
}
