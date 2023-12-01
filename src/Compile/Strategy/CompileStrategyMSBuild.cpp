/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyMSBuild.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/VSSolutionProjectExporter.hpp"
#include "Platform/Arch.hpp"
#include "Process/Process.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileStrategyMSBuild::CompileStrategyMSBuild(BuildState& inState) :
	ICompileStrategy(StrategyType::MSBuild, inState)
{
}

/*****************************************************************************/
bool CompileStrategyMSBuild::initialize()
{
	if (m_initialized)
		return false;

	// auto& cacheFile = m_state.cache.file();
	const auto& cachePathId = m_state.cachePathId();
	UNUSED(m_state.cache.getCachePath(cachePathId));

	// const bool buildStrategyChanged = cacheFile.buildStrategyChanged();

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool CompileStrategyMSBuild::addProject(const SourceTarget& inProject)
{
	return ICompileStrategy::addProject(inProject);
}

/*****************************************************************************/
bool CompileStrategyMSBuild::doFullBuild()
{
	// msbuild -nologo -t:Clean,Build -verbosity:m -clp:ForceConsoleColor -property:Configuration=Debug -property:Platform=x64 build/.projects/project.sln

	auto& route = m_state.inputs.route();
	auto& cwd = m_state.inputs.workingDirectory();

	auto msbuild = Files::which("msbuild");
	if (msbuild.empty())
	{
		Diagnostic::error("MSBuild is required, but was not found in path.");
		return false;
	}

	VSSolutionProjectExporter exporter(m_state.inputs);

	// TODO: In a recent version of MSBuild (observed in 17.6.3), there's an extra line break in minimal verbosity mode.
	//   Unsure if it's intentional, or a bug, but we'll heandle it for now
	//
	if (m_state.toolchain.versionMajorMinor() >= 1706 && !Output::showCommands())
		Output::previousLine();

	StringList cmd{
		msbuild,
		"-nologo",
		"-clp:ForceConsoleColor",
	};

	auto maxJobs = m_state.info.maxJobs();
	if (maxJobs > 1)
		cmd.emplace_back(fmt::format("-m:{}", maxJobs));

	if (!Output::showCommands())
		cmd.emplace_back("-verbosity:m");

	const auto& configuration = m_state.configuration.name();
	cmd.emplace_back(fmt::format("-property:Configuration={}", configuration));

	auto arch = Arch::toVSArch2(m_state.info.targetArchitecture());
	cmd.emplace_back(fmt::format("-property:Platform={}", arch));

	std::string target;
	if (route.isClean())
		target = "Clean";
	else if (route.isRebuild())
		target = "Clean,Build";
	else
		target = "Build";

	cmd.emplace_back(fmt::format("-target:{}", target));

	auto project = exporter.getMainProjectOutput(m_state);

	if (m_state.inputs.route().isBundle())
	{
		cmd.emplace_back(project);
	}
	else
	{
		auto folder = String::getPathFolder(project);
		cmd.emplace_back(fmt::format("{}/vcxproj/all.vcxproj", folder));
	}

	bool result = Process::run(cmd);
	if (result)
	{
		String::replaceAll(project, fmt::format("{}/", cwd), "");
		Output::msgAction("Succeeded", project);
	}

	return result;
}

/*****************************************************************************/
bool CompileStrategyMSBuild::buildProject(const SourceTarget& inProject)
{
	UNUSED(inProject);
	return true;
}

}
