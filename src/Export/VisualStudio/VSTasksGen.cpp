/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudio/VSTasksGen.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
VSTasksGen::VSTasksGen(const BuildState& inState, const std::string& inCwd) :
	m_state(inState),
	m_cwd(inCwd)
{
	UNUSED(m_state, m_cwd);
}

/*****************************************************************************/
bool VSTasksGen::saveToFile(const std::string& inFilename)
{
	Json jRoot;
	jRoot = Json::object();
	jRoot["version"] = "0.2.1";
	jRoot["tasks"] = Json::array();

	auto& tasks = jRoot.at("tasks");

	auto makeTask = [](const char* inLabel, const char* inContextType, const char* inChaletCmd) {
		Json task = Json::object();
		task["taskLabel"] = inLabel;
		task["appliesTo"] = "*";
		task["type"] = "launch";
		task["contextType"] = inContextType;
		task["inheritEnvironments"] = {
			"${cpp.activeConfiguration}",
		};
		task["workingDirectory"] = "${workspaceRoot}";
		task["command"] = "chalet";
		task["args"] = {
			"-c",
			"${chalet.configuration}",
			inChaletCmd,
		};

		return task;
	};

	tasks.emplace_back(makeTask("Chalet: Build / Run", "custom", "buildrun"));
	tasks.emplace_back(makeTask("Chalet: Run", "custom", "run"));
	tasks.emplace_back(makeTask("Build", "build", "build"));
	tasks.emplace_back(makeTask("Rebuild", "rebuild", "rebuild"));
	tasks.emplace_back(makeTask("Clean", "clean", "clean"));
	tasks.emplace_back(makeTask("Chalet: Bundle", "custom", "bundle"));
	tasks.emplace_back(makeTask("Chalet: Configure", "custom", "configure"));

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

}