/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSCode/VSCodeTasksGen.hpp"

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
VSCodeTasksGen::VSCodeTasksGen(const ExportAdapter& inExportAdapter) :
	m_exportAdapter(inExportAdapter)
{
}

/*****************************************************************************/
bool VSCodeTasksGen::saveToFile(const std::string& inFilename)
{
	if (!initialize())
		return false;

	Json jRoot;
	jRoot = Json::object();
	jRoot["version"] = "2.0.0";
	auto& tasks = jRoot["tasks"] = Json::array();

	for (auto& runConfig : m_runConfigs)
	{
		tasks.push_back(makeRunConfiguration(runConfig));
	}

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
// Note: We're calling Chalet, so we don't need the target's working directory, we need the cwd
//
Json VSCodeTasksGen::makeRunConfiguration(const ExportRunConfiguration& inRunConfig) const
{
	Json ret = Json::object();
	ret["label"] = m_exportAdapter.getRunConfigLabel(inRunConfig);
	ret["type"] = "process";
	ret["group"] = "build";
	ret["problemMatcher"] = {
		getProblemMatcher(),
	};
	ret["command"] = m_exportAdapter.getRunConfigExec();
	ret["args"] = m_exportAdapter.getRunConfigArguments(inRunConfig);

	return ret;
}

/*****************************************************************************/
bool VSCodeTasksGen::initialize()
{
	m_runConfigs = m_exportAdapter.getBasicRunConfigs();

	m_usesMsvc = willUseMSVC(m_exportAdapter.getDebugState());

	return true;
}

/*****************************************************************************/
std::string VSCodeTasksGen::getProblemMatcher() const
{
	if (m_usesMsvc)
		return std::string("$msCompile");
	else
		return std::string("$gcc");
}

/*****************************************************************************/
bool VSCodeTasksGen::willUseMSVC(const BuildState& inState) const
{
	return inState.environment->isMsvc();
}
}
