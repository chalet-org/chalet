/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Archiver/ArchiverGNUAR.hpp"

namespace chalet
{
struct ArchiverLLVMAR : public ArchiverGNUAR
{
	explicit ArchiverLLVMAR(const BuildState& inState, const SourceTarget& inProject);
};
}
