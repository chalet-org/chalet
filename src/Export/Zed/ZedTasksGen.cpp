/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Zed/ZedTasksGen.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ZedTasksGen::ZedTasksGen(const ExportAdapter& inExportAdapter) :
	m_exportAdapter(inExportAdapter)
{
}

/*****************************************************************************/
bool ZedTasksGen::saveToFile(const std::string& inFilename)
{
	if (!initialize())
		return false;

	Json jRoot;
	jRoot = Json::array();

	for (auto& runConfig : m_runConfigs)
	{
		jRoot.push_back(makeRunConfiguration(runConfig));
	}

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
// Note: We're calling Chalet, so we don't need the target's working directory, we need the cwd
//
Json ZedTasksGen::makeRunConfiguration(const ExportRunConfiguration& inRunConfig) const
{
	Json ret = Json::object();
	ret["label"] = m_exportAdapter.getRunConfigLabel(inRunConfig);
	auto& jTags = ret["tags"] = Json::array();
	jTags.emplace_back("build");

	ret["command"] = m_exportAdapter.getRunConfigExec();
	ret["args"] = m_exportAdapter.getRunConfigArguments(inRunConfig);

	return ret;
}

/*****************************************************************************/
bool ZedTasksGen::initialize()
{
	m_runConfigs = m_exportAdapter.getBasicRunConfigs();

	return true;
}
}
