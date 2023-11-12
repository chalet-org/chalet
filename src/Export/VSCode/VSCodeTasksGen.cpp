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
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
VSCodeTasksGen::VSCodeTasksGen(const BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
bool VSCodeTasksGen::saveToFile(const std::string& inFilename) const
{
	Json jRoot;
	jRoot = Json::object();
	jRoot["version"] = "2.0.0";
	jRoot["tasks"] = Json::array();
	auto& tasks = jRoot.at("tasks");

	tasks.push_back(getTask());

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
Json VSCodeTasksGen::getTask() const
{
	Json ret = Json::object();
	setLabel(ret);
	ret["type"] = "process";
	ret["command"] = "chalet";
	ret["args"] = {
		"-c",
		m_state.configuration.name(),
		"build"
	};
	ret["group"] = "build";
	ret["problemMatcher"] = {
		getProblemMatcher(),
	};

	return ret;
}

/*****************************************************************************/
void VSCodeTasksGen::setLabel(Json& outJson) const
{
	outJson["label"] = fmt::format("Build: {}", m_state.configuration.name());
}

/*****************************************************************************/
std::string VSCodeTasksGen::getProblemMatcher() const
{
	if (willUseMSVC())
		return std::string("$msCompile");
	else
		return std::string("$gcc");
}

/*****************************************************************************/
bool VSCodeTasksGen::willUseMSVC() const
{
	return m_state.environment->isMsvc() || m_state.environment->isWindowsClang();
}
}
