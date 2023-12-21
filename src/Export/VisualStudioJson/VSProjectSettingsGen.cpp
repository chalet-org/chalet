/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudioJson/VSProjectSettingsGen.hpp"

#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
VSProjectSettingsGen::VSProjectSettingsGen(const ExportAdapter& inExportAdapter) :
	m_exportAdapter(inExportAdapter)
{
}

/*****************************************************************************/
bool VSProjectSettingsGen::saveToFile(const std::string& inFilename)
{
	Json jRoot;
	jRoot = Json::object();
	jRoot["CurrentProjectSetting"] = m_exportAdapter.getDefaultTargetName();

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

}
