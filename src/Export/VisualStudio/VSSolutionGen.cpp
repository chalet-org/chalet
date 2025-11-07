/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
 */

#include "Export/VisualStudio/VSSolutionGen.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonValues.hpp"

namespace chalet
{
namespace
{
struct VisualStudioConfig
{
	ExportRunConfiguration* runConfig = nullptr;
	std::string arch;
	std::string vsArch;
	bool allTarget = false;
};
}

/*****************************************************************************/
VSSolutionGen::VSSolutionGen(const ExportAdapter& inExportAdapter, const std::string& inProjectTypeGuid, const OrderedDictionary<Uuid>& inTargetGuids) :
	m_exportAdapter(inExportAdapter),
	m_projectTypeGuid(inProjectTypeGuid),
	m_targetGuids(inTargetGuids)
{
}

/*****************************************************************************/
bool VSSolutionGen::saveToFile(const std::string& inFilename)
{
	auto runConfigs = m_exportAdapter.getBasicRunConfigs();
	if (runConfigs.empty())
		return false;

	std::vector<VisualStudioConfig> vsConfigs;
	for (auto& runConfig : runConfigs)
	{
		auto arch2 = Arch::from(runConfig.arch);

		vsConfigs.emplace_back(VisualStudioConfig{
			&runConfig,
			Arch::toVSArch(arch2.val),
			Arch::toVSArch2(arch2.val),
			String::equals(Values::All, runConfig.name),
		});
	}

	std::string minimumVisualStudioVersion{ "10.0.40219.1" };

	const auto& debugState = m_exportAdapter.getDebugState();

	const auto& workspaceName = debugState.workspace.metadata().name();
	auto& visualStudioVersion = debugState.environment->detectedVersion();

	auto solutionGUID = Uuid::v5(fmt::format("{}_SOLUTION", workspaceName), m_projectTypeGuid).toUpperCase();

	const auto visualStudioVersionMajor = debugState.environment->getMajorVersion();

	std::string runTargetName;
	const IBuildTarget* runnableTarget = debugState.getFirstValidRunTarget(true);
	if (runnableTarget != nullptr)
	{
		runTargetName = runnableTarget->name();
	}

	std::string vsConfigString;
	std::string vsProjectsString;

	auto addProjectToSolution = [this, &vsProjectsString](const std::string& name, const std::string& projectGUID) {
		vsProjectsString += fmt::format("Project(\"{{{projectTypeGUID}}}\") = \"{name}\", \"vcxproj/{name}.vcxproj\", \"{{{projectGUID}}}\"\n",
			fmt::arg("projectTypeGUID", m_projectTypeGuid),
			FMT_ARG(projectGUID),
			FMT_ARG(name));

		vsProjectsString += "EndProject\n";
	};

	bool runTargetFound = false;
	if (!runTargetName.empty())
	{
		for (auto& [name, guid] : m_targetGuids)
		{
			if (String::equals(name, runTargetName))
			{
				addProjectToSolution(name, guid.toUpperCase());
				runTargetFound = true;
			}
		}
	}

	if (!vsConfigs.empty())
	{
		std::string configs;
		std::string projectConfigs;

		for (auto& conf : vsConfigs)
		{
			if (conf.allTarget)
				continue;

			const auto& config = conf.runConfig->config;
			const auto& arch = conf.arch;
			configs += fmt::format("\n\t\t{config}|{arch} = {config}|{arch}", FMT_ARG(config), FMT_ARG(arch));
		}

		for (auto& [name, guid] : m_targetGuids)
		{
			auto projectGUID = guid.toUpperCase();

			bool isRunTarget = runTargetFound && String::equals(name, runTargetName);
			if (!isRunTarget)
			{
				addProjectToSolution(name, projectGUID);
			}

			for (auto& conf : vsConfigs)
			{
				if (conf.allTarget)
					continue;

				const auto& config = conf.runConfig->config;
				const auto& arch = conf.arch;
				const auto& vsArch = conf.vsArch;

				auto state = m_exportAdapter.getStateFromRunConfig(*conf.runConfig);
				bool includedInBuild = projectWillBuild(name, *state);
				// bool includedInBuild = !allTarget;

				projectConfigs += fmt::format("\n\t\t{{{projectGUID}}}.{config}|{arch}.ActiveCfg = {config}|{vsArch}",
					FMT_ARG(projectGUID),
					FMT_ARG(config),
					FMT_ARG(arch),
					FMT_ARG(vsArch));

				if (includedInBuild)
				{
					projectConfigs += fmt::format("\n\t\t{{{projectGUID}}}.{config}|{arch}.Build.0 = {config}|{vsArch}",
						FMT_ARG(projectGUID),
						FMT_ARG(config),
						FMT_ARG(arch),
						FMT_ARG(vsArch));
				}
			}
		}

		vsConfigString += fmt::format("\n\tGlobalSection(SolutionConfigurationPlatforms) = preSolution{configs}\n\tEndGlobalSection", FMT_ARG(configs));
		vsConfigString += fmt::format("\n\tGlobalSection(ProjectConfigurationPlatforms) = postSolution{projectConfigs}\n\tEndGlobalSection", FMT_ARG(projectConfigs));
	}

	std::string contents = String::withByteOrderMark(fmt::format(R"sln(
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version {visualStudioVersionMajor}
VisualStudioVersion = {visualStudioVersion}
MinimumVisualStudioVersion = {minimumVisualStudioVersion}
{vsProjectsString}Global{vsConfigString}
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
	GlobalSection(ExtensibilityGlobals) = postSolution
		SolutionGuid = {{{solutionGUID}}}
	EndGlobalSection
EndGlobal)sln",
		FMT_ARG(visualStudioVersionMajor),
		FMT_ARG(visualStudioVersion),
		FMT_ARG(minimumVisualStudioVersion),
		FMT_ARG(vsProjectsString),
		FMT_ARG(solutionGUID),
		FMT_ARG(vsConfigString)));

	if (!Files::createFileWithContents(inFilename, contents))
	{
		Diagnostic::error("There was a problem creating the VS solution: {}", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool VSSolutionGen::projectWillBuild(const std::string& inName, const BuildState& inState) const
{
	for (auto& target : inState.targets)
	{
		if (String::equals(inName, target->name()))
			return true;
	}

	return false;
}
}
