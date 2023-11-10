/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Archiver/ArchiverVisualStudioLIB.hpp"

namespace chalet
{
struct ArchiverIntelClassicLIB : public ArchiverVisualStudioLIB
{
	explicit ArchiverIntelClassicLIB(const BuildState& inState, const SourceTarget& inProject);
};
}
