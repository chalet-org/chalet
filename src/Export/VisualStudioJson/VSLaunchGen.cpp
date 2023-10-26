/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudioJson/VSLaunchGen.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
VSLaunchGen::VSLaunchGen(const std::vector<Unique<BuildState>>& inStates) :
	m_states(inStates)
{
}

/*****************************************************************************/
bool VSLaunchGen::saveToFile(const std::string& inFilename)
{
	Json jRoot;
	jRoot = Json::object();
	jRoot["version"] = "3.0.0";
	jRoot["defaults"] = Json::object();
	jRoot["configurations"] = Json::array();
	auto& configurations = jRoot.at("configurations");

	StringList targetList;
	for (const auto& state : m_states)
	{
		m_executableTargets.clear();
		for (auto& target : state->targets)
		{
			if (target->isSources())
			{
				const auto& project = static_cast<const SourceTarget&>(*target);
				if (project.isExecutable())
				{
					m_executableTargets.emplace_back(target.get());
				}
			}
			else if (target->isCMake())
			{
				const auto& cmakeProject = static_cast<const CMakeTarget&>(*target);
				if (!cmakeProject.runExecutable().empty())
				{
					m_executableTargets.emplace_back(target.get());
				}
			}
		}

		for (auto target : m_executableTargets)
		{
			if (!List::contains(targetList, target->name()))
			{
				configurations.emplace_back(getConfiguration(*state, *target));
				targetList.emplace_back(target->name());
			}
		}
	}

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
Json VSLaunchGen::getConfiguration(const BuildState& inState, const IBuildTarget& inTarget) const
{
	const auto& targetName = inTarget.name();
	const auto& runArgumentMap = inState.getCentralState().runArgumentMap();

	StringList arguments;
	if (runArgumentMap.find(targetName) != runArgumentMap.end())
	{
		arguments = String::split(runArgumentMap.at(targetName), ' ');
	}

	Json ret = Json::object();

	auto program = inState.paths.getExecutableTargetPath(inTarget);
	// String::replaceAll(program, inState.configuration.name(), "${env.CHALET_CONFIG}");

	auto filename = String::getPathFilename(program);
	ret["name"] = filename;
	// ret["project"] = fmt::format("${{chalet.buildDir}}/{}", filename);
	ret["project"] = Commands::getCanonicalPath(program);
	ret["args"] = arguments;

	ret["currentDir"] = "${workspaceRoot}";
	ret["debugType"] = "native";
	ret["stopOnEntry"] = true;

	ret["env"] = getEnvironment(inTarget);
	ret["inheritEnvironments"] = {
		"${cpp.activeConfiguration}",
	};

	return ret;
}

/*****************************************************************************/
Json VSLaunchGen::getEnvironment(const IBuildTarget& inTarget) const
{
	Json ret = Json::object();
	ret["Path"] = "${chalet.runEnvironment};${env.Path}";
	// ret["CHALET_CONFIG"] = "${chalet.configuration}";

	UNUSED(inTarget);

	return ret;
}

}
