/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Xcode/XcodeProjectSpecGen.hpp"

namespace chalet
{
/*****************************************************************************/
XcodeProjectSpecGen::XcodeProjectSpecGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inExportDir) :
	m_states(inStates),
	m_exportDir(inExportDir)
{
	UNUSED(m_states, m_exportDir);
}

/*****************************************************************************/
bool XcodeProjectSpecGen::saveToFile(const std::string& inFilename) const
{
	LOG(inFilename);

	return true;
}
}
