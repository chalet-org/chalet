/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSCode/VSCodeLaunchGen.hpp"

#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
VSCodeLaunchGen::VSCodeLaunchGen(const BuildState& inState, const std::string& inCwd) :
	m_state(inState),
	m_cwd(inCwd)
{
	UNUSED(m_state, m_cwd);
}

/*****************************************************************************/
bool VSCodeLaunchGen::saveToFile(const std::string& inFilename) const
{
	Json jRoot;
	jRoot = Json::object();
	jRoot["version"] = "0.2.0";
	jRoot["configurations"] = Json::array();

	Json config = Json::object();

	jRoot["configurations"].push_back(std::move(config));

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}
}
