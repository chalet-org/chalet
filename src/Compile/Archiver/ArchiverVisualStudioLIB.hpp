/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ARCHIVER_VISUAL_STUDIO_LIB_HPP
#define CHALET_ARCHIVER_VISUAL_STUDIO_LIB_HPP

#include "Compile/Archiver/IArchiver.hpp"

namespace chalet
{
struct ArchiverVisualStudioLIB : public IArchiver
{
	explicit ArchiverVisualStudioLIB(const BuildState& inState, const SourceTarget& inProject);

	virtual StringList getCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) const override;

protected:
	virtual void addTargetPlatformArch(StringList& outArgList) const final;
};
}

#endif // CHALET_ARCHIVER_VISUAL_STUDIO_LIB_HPP
