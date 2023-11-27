/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Archiver/IArchiver.hpp"

namespace chalet
{
struct ArchiverGNUAR : public IArchiver
{
	explicit ArchiverGNUAR(const BuildState& inState, const SourceTarget& inProject);

	virtual StringList getCommand(const std::string& outputFile, const StringList& sourceObjs) const override;
};
}
