/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudioJson/VSLaunchGen.hpp"

#include "Export/TargetExportAdapter.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/MesonTarget.hpp"
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
	auto& configurations = jRoot["configurations"] = Json::array();

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
					executableTargets.emplace_back(target.get());
			}
			else if (target->isCMake())
			{
				const auto& project = static_cast<const CMakeTarget&>(*target);
				if (!project.runExecutable().empty())
					executableTargets.emplace_back(target.get());
			}
			else if (target->isMeson())
			{
				const auto& project = static_cast<const MesonTarget&>(*target);
				if (!project.runExecutable().empty())
					executableTargets.emplace_back(target.get());
			}
		}

		for (auto target : executableTargets)
		{
			Json configuration;
			if (!getConfiguration(configuration, runConfig, *state, *target))
				return false;

			configurations.emplace_back(std::move(configuration));
		}
	}

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
bool VSLaunchGen::getConfiguration(Json& outConfiguration, const ExportRunConfiguration& runConfig, const BuildState& inState, const IBuildTarget& inTarget) const
{
	StringList arguments;
	if (!inState.getRunTargetArguments(arguments, &inTarget))
		return false;

	TargetExportAdapter adapter(inState, inTarget);
	outConfiguration = Json::object();

	outConfiguration["name"] = m_exportAdapter.getRunConfigLabel(runConfig);
	outConfiguration["project"] = Files::getCanonicalPath(runConfig.outputFile);
	outConfiguration["args"] = arguments;

	outConfiguration["currentDir"] = adapter.getRunWorkingDirectoryWithCurrentWorkingDirectoryAs("${workspaceRoot}");
	outConfiguration["debugType"] = "native";
	outConfiguration["stopOnEntry"] = true;

	outConfiguration["env"] = getEnvironment(inTarget);
	outConfiguration["inheritEnvironments"] = {
		"${cpp.activeConfiguration}",
	};

	return true;
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
