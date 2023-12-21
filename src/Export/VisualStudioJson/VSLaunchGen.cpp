/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudioJson/VSLaunchGen.hpp"

#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
VSLaunchGen::VSLaunchGen(const ExportAdapter& inExportAdapter) :
	m_exportAdapter(inExportAdapter)
{
}

/*****************************************************************************/
bool VSLaunchGen::saveToFile(const std::string& inFilename)
{
	m_runConfigs = m_exportAdapter.getFullRunConfigs();

	Json jRoot;
	jRoot = Json::object();
	jRoot["version"] = "3.0.0";
	jRoot["defaults"] = Json::object();
	jRoot["configurations"] = Json::array();
	auto& configurations = jRoot.at("configurations");

	auto& allTarget = m_exportAdapter.allBuildName();
	for (auto& runConfig : m_runConfigs)
	{
		if (String::equals(allTarget, runConfig.name))
			continue;

		std::vector<const IBuildTarget*> executableTargets;
		auto state = m_exportAdapter.getStateFromRunConfig(runConfig);
		for (auto& target : state->targets)
		{
			if (target->isSources())
			{
				const auto& project = static_cast<const SourceTarget&>(*target);
				if (project.isExecutable())
				{
					executableTargets.emplace_back(target.get());
				}
			}
			else if (target->isCMake())
			{
				const auto& cmakeProject = static_cast<const CMakeTarget&>(*target);
				if (!cmakeProject.runExecutable().empty())
				{
					executableTargets.emplace_back(target.get());
				}
			}
		}

		for (auto target : executableTargets)
		{
			configurations.emplace_back(getConfiguration(runConfig, state->getCentralState(), *target));
		}
	}

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
Json VSLaunchGen::getConfiguration(const RunConfiguration& runConfig, const CentralState& inCentralState, const IBuildTarget& inTarget) const
{
	const auto& targetName = inTarget.name();
	const auto& runArgumentMap = inCentralState.runArgumentMap();

	StringList arguments;
	if (runArgumentMap.find(targetName) != runArgumentMap.end())
	{
		arguments = String::split(runArgumentMap.at(targetName), ' ');
	}

	Json ret = Json::object();

	ret["name"] = m_exportAdapter.getRunConfigLabel(runConfig);
	ret["project"] = Files::getCanonicalPath(runConfig.outputFile);
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
