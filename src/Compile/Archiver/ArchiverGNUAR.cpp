/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Archiver/ArchiverGNUAR.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
ArchiverGNUAR::ArchiverGNUAR(const BuildState& inState, const SourceTarget& inProject) :
	IArchiver(inState, inProject)
{
}

/*****************************************************************************/
StringList ArchiverGNUAR::getCommand(const std::string& outputFile, const StringList& sourceObjs) const
{
	StringList ret;

	if (m_state.toolchain.archiver().empty())
		return ret;

	ret.emplace_back(getQuotedPath(m_state.toolchain.archiver()));

	ret.emplace_back("-c");
	ret.emplace_back("-r");
	ret.emplace_back("-s");

	ret.emplace_back(getQuotedPath(outputFile));
	addSourceObjects(ret, sourceObjs);

	return ret;
}
}
