/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudio/VSLaunchGen.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
VSLaunchGen::VSLaunchGen(const BuildState& inState, const std::string& inCwd, const IBuildTarget& inTarget) :
	m_state(inState),
	m_cwd(inCwd),
	m_target(inTarget)
{
	UNUSED(m_cwd);
}

/*****************************************************************************/
bool VSLaunchGen::saveToFile(const std::string& inFilename) const
{
	Json jRoot;
	jRoot = Json::object();
	jRoot["version"] = "3.0.0";
	jRoot["defaults"] = Json::object();
	jRoot["configurations"] = Json::array();
	auto& configurations = jRoot.at("configurations");

	configurations.push_back(getConfiguration());

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
Json VSLaunchGen::getConfiguration() const
{
	Json ret = Json::object();
	ret["type"] = "default";

	auto filename = String::getPathFilename(m_state.paths.getExecutableTargetPath(m_target));
	ret["projectTarget"] = filename;
	ret["name"] = filename;

	ret["currentDir"] = "${projectDir}";
	ret["env"] = getEnvironment();
	ret["inheritEnvironments"] = {
		getInheritEnvironment(),
	};

	if (m_state.inputs.runArguments().has_value())
		ret["args"] = *m_state.inputs.runArguments();
	else
		ret["args"] = Json::array();

	return ret;
}

/*****************************************************************************/
Json VSLaunchGen::getEnvironment() const
{
	Json ret = Json::object();
	ret["Path"] = "${env.Path}";

	return ret;
}

/*****************************************************************************/
std::string VSLaunchGen::getInheritEnvironment() const
{
	return std::string("msvc_x64_x64");
}

}
