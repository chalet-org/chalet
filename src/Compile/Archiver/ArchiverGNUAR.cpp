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
StringList ArchiverGNUAR::getCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) const
{
	UNUSED(outputFileBase);

	StringList ret;

	if (m_state.toolchain.archiver().empty())
		return ret;

	ret.emplace_back(getQuotedExecutablePath(m_state.toolchain.archiver()));

	ret.emplace_back("-c");
	ret.emplace_back("-r");
	ret.emplace_back("-s");

	ret.push_back(outputFile);
	addSourceObjects(ret, sourceObjs);

	return ret;
}
}
