/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
 */

#include "Export/VisualStudio/VSSolutionGen.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
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
VSSolutionGen::VSSolutionGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inCwd) :
	m_states(inStates),
	m_cwd(inCwd)
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

	std::string solutionGUID{ "7838F8F8-6365-4CC0-AF17-1F1C13E21FA2" };
	std::string projectTypeGUID{ "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942" };
	std::string projectGUID{ "24303D41-C0B0-4E37-9656-1AD5883A7B9B" };

	std::string minimumVisualStudioVersion{ "10.0.40219.1" };

	const auto& firstState = *m_states.front();
	const auto& workspaceName = firstState.workspace.metadata().name();
	auto& visualStudioVersion = firstState.environment->detectedVersion();

	std::string visualStudioVersionMajor = visualStudioVersion;
	{
		auto firstDecimal = visualStudioVersionMajor.find('.');
		if (firstDecimal != std::string::npos)
		{
			visualStudioVersionMajor = visualStudioVersionMajor.substr(0, firstDecimal);
		}
	}

	std::string vsConfigString;
	if (!vsConfigs.empty())
	{
		std::string configs;
		std::string projectConfigs;
		for (auto& [name, arch, arch2] : vsConfigs)
		{
			configs += fmt::format("\n\t\t{name}|{arch} = {name}|{arch}", FMT_ARG(name), FMT_ARG(arch));
			projectConfigs += fmt::format("\n\t\t{{{projectGUID}}}.{name}|{arch}.ActiveCfg = {name}|{arch2}",
				FMT_ARG(projectGUID),
				FMT_ARG(name),
				FMT_ARG(arch),
				FMT_ARG(arch2));
			projectConfigs += fmt::format("\n\t\t{{{projectGUID}}}.{name}|{arch}.Build.0 = {name}|{arch2}",
				FMT_ARG(projectGUID),
				FMT_ARG(name),
				FMT_ARG(arch),
				FMT_ARG(arch2));
		}

		vsConfigString += fmt::format("\n\tGlobalSection(SolutionConfigurationPlatforms) = preSolution{configs}\n\tEndGlobalSection", FMT_ARG(configs));
		vsConfigString += fmt::format("\n\tGlobalSection(ProjectConfigurationPlatforms) = postSolution{projectConfigs}\n\tEndGlobalSection", FMT_ARG(projectConfigs));
	}

	std::string contents = fmt::format(R"sln(
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version {visualStudioVersionMajor}
VisualStudioVersion = {visualStudioVersion}
MinimumVisualStudioVersion = {minimumVisualStudioVersion}
Project("{{{projectTypeGUID}}}") = "{workspaceName}", "{workspaceName}/{workspaceName}.vcxproj", "{{{projectGUID}}}"
EndProject
Global{vsConfigString}
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
		FMT_ARG(projectGUID),
		FMT_ARG(projectTypeGUID),
		FMT_ARG(solutionGUID),
		FMT_ARG(vsConfigString),
		FMT_ARG(workspaceName));

	UNUSED(inFilename);

	return Commands::createFileWithContents(inFilename, contents);
}
}
