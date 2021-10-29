/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ARCHIVER_LIB_TOOL_HPP
#define CHALET_ARCHIVER_LIB_TOOL_HPP

#include "Compile/Archiver/IArchiver.hpp"

namespace chalet
{
struct ArchiverLibTool final : public IArchiver
{
	explicit ArchiverLibTool(const BuildState& inState, const SourceTarget& inProject);

	virtual StringList getCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) const final;
};
}

#endif // CHALET_ARCHIVER_LIB_TOOL_HPP
