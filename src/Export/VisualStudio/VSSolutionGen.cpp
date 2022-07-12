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
VSSolutionGen::VSSolutionGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inCwd, const std::string& inProjectTypeGuid, const OrderedDictionary<Uuid>& inTargetGuids) :
	m_states(inStates),
	m_cwd(inCwd),
	m_projectTypeGuid(inProjectTypeGuid),
	m_targetGuids(inTargetGuids)
{
	UNUSED(m_cwd);
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
		std::string arch2 = arch;
		if (String::equals("x86", arch2))
			arch2 = "Win32";

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

	auto solutionGUID = Uuid::v5(workspaceName, m_projectTypeGuid).toUpperCase();

	std::string visualStudioVersionMajor = visualStudioVersion;
	{
		auto firstDecimal = visualStudioVersionMajor.find('.');
		if (firstDecimal != std::string::npos)
		{
			visualStudioVersionMajor = visualStudioVersionMajor.substr(0, firstDecimal);
		}
	}

	std::string vsConfigString;
	std::string vsProjectsString;

	if (!vsConfigs.empty())
	{
		std::string configs;
		std::string projectConfigs;

		for (auto& [name, arch, arch2] : vsConfigs)
		{
			UNUSED(arch, arch2);
			// configs += fmt::format("\n\t\t{name}|{arch} = {name}|{arch}", FMT_ARG(name), FMT_ARG(arch));
			std::string arch3{ "Any CPU" };
			configs += fmt::format("\n\t\t{name}|{arch3} = {name}|{arch3}", FMT_ARG(name), FMT_ARG(arch3));
		}

		for (auto& [name, guid] : m_targetGuids)
		{
			auto projectGUID = guid.toUpperCase();
			vsProjectsString += fmt::format("Project(\"{{{projectTypeGUID}}}\") = \"{name}\", \"{name}/{name}.vcxproj\", \"{{{projectGUID}}}\"\n",
				fmt::arg("projectTypeGUID", m_projectTypeGuid),
				FMT_ARG(projectGUID),
				FMT_ARG(name));

			vsProjectsString += "EndProject\n";

			for (auto& [conf, arch, arch2] : vsConfigs)
			{
				projectConfigs += fmt::format("\n\t\t{{{projectGUID}}}.{conf}|{arch}.ActiveCfg = {conf}|{arch2}",
					FMT_ARG(projectGUID),
					FMT_ARG(conf),
					FMT_ARG(arch),
					FMT_ARG(arch2));
				projectConfigs += fmt::format("\n\t\t{{{projectGUID}}}.{conf}|{arch}.Build.0 = {conf}|{arch2}",
					FMT_ARG(projectGUID),
					FMT_ARG(conf),
					FMT_ARG(arch),
					FMT_ARG(arch2));
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

	return Commands::createFileWithContents(inFilename, contents);
}
}
