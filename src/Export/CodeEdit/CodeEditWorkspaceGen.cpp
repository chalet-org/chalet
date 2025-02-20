/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CodeEdit/CodeEditWorkspaceGen.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
CodeEditWorkspaceGen::CodeEditWorkspaceGen(const ExportAdapter& inExportAdapter) :
	m_exportAdapter(inExportAdapter)
{
}

/*****************************************************************************/
bool CodeEditWorkspaceGen::saveToPath(const std::string& inPath)
{
	m_runConfigs = m_exportAdapter.getBasicRunConfigs();
	m_exportAdapter.createCompileCommandsStub();

	auto settingsJson = fmt::format("{}/settings.json", inPath);
	if (!createSettingsJsonFile(settingsJson))
		return false;

	return true;
}

/*****************************************************************************/
bool CodeEditWorkspaceGen::createSettingsJsonFile(const std::string& inFilename)
{
	Json jRoot;
	jRoot = Json::object();

	auto& state = m_exportAdapter.getDebugState();

	jRoot["project"] = Json::object();
	jRoot["project"]["projectName"] = state.workspace.metadata().name();

	jRoot["tasks"] = Json::array();
	for (auto& runConfig : m_runConfigs)
	{
		// runConfig.
		jRoot["tasks"].emplace_back(makeRunTask(runConfig));
	}

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
Json CodeEditWorkspaceGen::makeRunTask(const RunConfiguration& inRunConfig) const
{
	Json ret;
	ret["name"] = m_exportAdapter.getRunConfigLabel(inRunConfig);
	ret["workingDirectory"] = m_exportAdapter.cwd();

	auto command = m_exportAdapter.getRunConfigExec();
	auto arguments = m_exportAdapter.getRunConfigArguments(inRunConfig);
	command += fmt::format(" {}", String::join(arguments));
	ret["command"] = std::move(command);

	return ret;
}

}