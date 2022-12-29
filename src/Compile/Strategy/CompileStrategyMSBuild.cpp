/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyMSBuild.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Core/Arch.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/IProjectExporter.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"

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

	auto& cacheFile = m_state.cache.file();
	const auto& cachePathId = m_state.cachePathId();
	UNUSED(m_state.cache.getCachePath(cachePathId, CacheType::Local));

	const bool buildStrategyChanged = cacheFile.buildStrategyChanged();
	if (buildStrategyChanged)
	{
		Commands::removeRecursively(m_state.paths.buildOutputDir());
	}

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

	auto msbuild = Commands::which("msbuild");
	if (msbuild.empty())
	{
		Diagnostic::error("MSBuild is required, but was not found in path.");
		return false;
	}

	StringList cmd{
		msbuild,
		"-nologo",
		"-clp:ForceConsoleColor",
	};

	if (!Output::showCommands())
		cmd.emplace_back("-verbosity:m");

	const auto& configuration = m_state.configuration.name();
	cmd.emplace_back(fmt::format("-property:Configuration={}", configuration));

	auto arch = Arch::toVSArch2(m_state.info.hostArchitecture());
	cmd.emplace_back(fmt::format("-property:Platform={}", arch));

	std::string target;
	if (route.isClean())
		target = "Clean";
	else if (route.isRebuild())
		target = "Clean,Build";
	else
		target = "Build";

	cmd.emplace_back(fmt::format("-target:{}", target));

	auto directory = IProjectExporter::getProjectBuildFolder(m_state.inputs);
	cmd.emplace_back(fmt::format("{}/project.sln", directory));

	bool result = Commands::subprocess(cmd);

	return result;
}

/*****************************************************************************/
bool CompileStrategyMSBuild::buildProject(const SourceTarget& inProject)
{
	UNUSED(inProject);
	return true;
}

}
