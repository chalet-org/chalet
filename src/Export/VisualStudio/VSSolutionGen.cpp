/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
 */

#include "Export/VisualStudio/VSSolutionGen.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
struct VisualStudioConfig
{
	std::string name;
	std::string arch;
	std::string arch2;
};
}

/*****************************************************************************/
VSSolutionGen::VSSolutionGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inProjectTypeGuid, const OrderedDictionary<Uuid>& inTargetGuids) :
	m_states(inStates),
	m_projectTypeGuid(inProjectTypeGuid),
	m_targetGuids(inTargetGuids)
{
}

/*****************************************************************************/
bool VSSolutionGen::saveToFile(const std::string& inFilename)
{
	if (m_states.empty())
		return false;

	std::vector<VisualStudioConfig> vsConfigs;
	for (auto& state : m_states)
	{
		std::string arch = Arch::toVSArch(state->info.targetArchitecture());
		std::string arch2 = Arch::toVSArch2(state->info.targetArchitecture());

		vsConfigs.emplace_back(VisualStudioConfig{
			state->configuration.name(),
			std::move(arch),
			std::move(arch2),
		});
	}

	std::string minimumVisualStudioVersion{ "10.0.40219.1" };

	const auto& firstState = *m_states.front();
	const auto& workspaceName = firstState.workspace.metadata().name();
	auto& visualStudioVersion = firstState.environment->detectedVersion();

	auto solutionGUID = Uuid::v5(fmt::format("{}_SOLUTION", workspaceName), m_projectTypeGuid).toUpperCase();

	const auto visualStudioVersionMajor = firstState.environment->getMajorVersion();

	std::string vsConfigString;
	std::string vsProjectsString;

	if (!vsConfigs.empty())
	{
		std::string configs;
		std::string projectConfigs;

		for (auto& [name, arch, arch2] : vsConfigs)
		{
			UNUSED(arch2);
			configs += fmt::format("\n\t\t{name}|{arch} = {name}|{arch}", FMT_ARG(name), FMT_ARG(arch));
		}

		for (auto& [name, guid] : m_targetGuids)
		{
			auto projectGUID = guid.toUpperCase();
			vsProjectsString += fmt::format("Project(\"{{{projectTypeGUID}}}\") = \"{name}\", \"vcxproj/{name}.vcxproj\", \"{{{projectGUID}}}\"\n",
				fmt::arg("projectTypeGUID", m_projectTypeGuid),
				FMT_ARG(projectGUID),
				FMT_ARG(name));

			vsProjectsString += "EndProject\n";

			for (auto& [conf, arch, arch2] : vsConfigs)
			{
				bool includedInBuild = projectWillBuild(name, conf);

				projectConfigs += fmt::format("\n\t\t{{{projectGUID}}}.{conf}|{arch}.ActiveCfg = {conf}|{arch2}",
					FMT_ARG(projectGUID),
					FMT_ARG(conf),
					FMT_ARG(arch),
					FMT_ARG(arch2));

				if (includedInBuild)
				{
					projectConfigs += fmt::format("\n\t\t{{{projectGUID}}}.{conf}|{arch}.Build.0 = {conf}|{arch2}",
						FMT_ARG(projectGUID),
						FMT_ARG(conf),
						FMT_ARG(arch),
						FMT_ARG(arch2));
				}
			}
		}

		vsConfigString += fmt::format("\n\tGlobalSection(SolutionConfigurationPlatforms) = preSolution{configs}\n\tEndGlobalSection", FMT_ARG(configs));
		vsConfigString += fmt::format("\n\tGlobalSection(ProjectConfigurationPlatforms) = postSolution{projectConfigs}\n\tEndGlobalSection", FMT_ARG(projectConfigs));
	}

	std::string contents = fmt::format(R"sln(
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
		FMT_ARG(vsConfigString));

	UNUSED(inFilename);

	if (!Commands::createFileWithContents(inFilename, contents))
	{
		Diagnostic::error("There was a problem creating the VS solution: {}", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool VSSolutionGen::projectWillBuild(const std::string& inName, const std::string& inConfigName) const
{
	for (auto& state : m_states)
	{
		if (String::equals(inConfigName, state->configuration.name()))
		{
			for (auto& target : state->targets)
			{
				if (String::equals(inName, target->name()))
					return true;
			}
		}
	}

	return false;
}
}
