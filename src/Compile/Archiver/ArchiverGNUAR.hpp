/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ARCHIVER_GNU_AR_HPP
#define CHALET_ARCHIVER_GNU_AR_HPP

#include "Compile/Archiver/IArchiver.hpp"

namespace chalet
{
struct ArchiverGNUAR : public IArchiver
{
	explicit ArchiverGNUAR(const BuildState& inState, const SourceTarget& inProject);

	virtual StringList getCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) const override;
};
}

#endif // CHALET_ARCHIVER_GNU_AR_HPP
