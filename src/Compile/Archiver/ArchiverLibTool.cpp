/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Archiver/ArchiverLibTool.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
ArchiverLibTool::ArchiverLibTool(const BuildState& inState, const SourceTarget& inProject) :
	IArchiver(inState, inProject)
{
}

/*****************************************************************************/
StringList ArchiverLibTool::getCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) const
{
	UNUSED(outputFileBase);

	StringList ret;

	if (m_state.toolchain.archiver().empty())
		return ret;

	ret.emplace_back(getQuotedPath(m_state.toolchain.archiver()));

	ret.emplace_back("-static");
	ret.emplace_back("-no_warning_for_no_symbols");

	ret.emplace_back("-o");
	ret.emplace_back(getQuotedPath(outputFile));

	addSourceObjects(ret, sourceObjs);

	return ret;
}
}
