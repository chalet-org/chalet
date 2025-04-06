/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Fleet/FleetWorkspaceGen.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Query/QueryController.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
FleetWorkspaceGen::FleetWorkspaceGen(const ExportAdapter& inExportAdapter) :
	m_exportAdapter(inExportAdapter)
{
}

/*****************************************************************************/
bool FleetWorkspaceGen::saveToPath(const std::string& inPath)
{
	m_runConfigs = m_exportAdapter.getBasicRunConfigs();
	m_exportAdapter.createCompileCommandsStub();

	auto runJson = fmt::format("{}/run.json", inPath);
	if (!createRunJsonFile(runJson))
		return false;

	auto settingsJson = fmt::format("{}/settings.json", inPath);
	if (!createSettingsJsonFile(settingsJson))
		return false;

	return true;
}

/*****************************************************************************/
bool FleetWorkspaceGen::createRunJsonFile(const std::string& inFilename)
{
	Json jRoot;
	jRoot = Json::object();

	jRoot["configurations"] = Json::array();

	for (auto& runConfig : m_runConfigs)
	{
		jRoot["configurations"].emplace_back(makeRunConfiguration(runConfig));
	}

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
bool FleetWorkspaceGen::createSettingsJsonFile(const std::string& inFilename)
{
	Json jRoot;
	jRoot = Json::object();

	const auto kClangdDatabases = "lsp.clangd.compilation.databases";

	jRoot[kClangdDatabases] = Json::array();

	auto& debugState = m_exportAdapter.getDebugState();
	auto compileCommands = Files::getCanonicalPath(fmt::format("{}/compile_commands.json", debugState.inputs.outputDirectory()));
	// #if defined(CHALET_WIN32)
	// 	Path::toWindows(compileCommands);
	// #endif
	jRoot[kClangdDatabases].emplace_back(compileCommands);

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
// Note: We're calling Chalet, so we don't need the target's working directory, we need the cwd
//
Json FleetWorkspaceGen::makeRunConfiguration(const ExportRunConfiguration& inRunConfig) const
{
	Json ret;
	ret["type"] = "command";
	ret["name"] = m_exportAdapter.getRunConfigLabel(inRunConfig);
	ret["workingDir"] = m_exportAdapter.cwd();

	ret["program"] = m_exportAdapter.getRunConfigExec();
	ret["args"] = m_exportAdapter.getRunConfigArguments(inRunConfig);

	return ret;
}

}