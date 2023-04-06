/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyXcodeBuild.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Core/Arch.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/IProjectExporter.hpp"
#include "State/AncillaryTools.hpp"
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
CompileStrategyXcodeBuild::CompileStrategyXcodeBuild(BuildState& inState) :
	ICompileStrategy(StrategyType::XcodeBuild, inState)
{
}

/*****************************************************************************/
bool CompileStrategyXcodeBuild::initialize()
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
bool CompileStrategyXcodeBuild::addProject(const SourceTarget& inProject)
{
	return ICompileStrategy::addProject(inProject);
}

/*****************************************************************************/
bool CompileStrategyXcodeBuild::doFullBuild()
{
	// LOG("   Hello XcodeBuild");

	// auto& route = m_state.inputs.route();

	auto xcodebuild = Commands::which("xcodebuild");
	if (xcodebuild.empty())
	{
		Diagnostic::error("Xcodebuild is required, but was not found in path.");
		return false;
	}

	StringList cmd{
		xcodebuild,
		// "-verbose",
		"-hideShellScriptEnvironment"
	};

	// if (!Output::showCommands())
	cmd.emplace_back("-verbose");

	cmd.emplace_back("-configuration");
	cmd.emplace_back(m_state.configuration.name());

	cmd.emplace_back("-arch");
	cmd.emplace_back(m_state.info.targetArchitectureString());

	// cmd.emplace_back("-destination");
	// cmd.emplace_back(fmt::format("platform=OS X,arch={}", m_state.info.targetArchitectureString()));

	cmd.emplace_back("-alltargets");
	// std::string target;
	// if (route.isClean())
	// 	target = "Clean";
	// else if (route.isRebuild())
	// 	target = "Clean,Build";
	// else
	// 	target = "Build";

	// cmd.emplace_back(fmt::format("-target:{}", target));

	cmd.emplace_back("-jobs");
	cmd.emplace_back(std::to_string(m_state.info.maxJobs()));

	cmd.emplace_back("-parallelizeTargets");

	cmd.emplace_back("-project");
	// cmd.emplace_back("project");
	auto directory = IProjectExporter::getProjectBuildFolder(m_state.inputs);
	cmd.emplace_back(fmt::format("{}/.xcode/project.xcodeproj", directory));

	const auto& signingIdentity = m_state.tools.signingIdentity();
	if (!signingIdentity.empty())
	{
		cmd.emplace_back(fmt::format("CODE_SIGN_IDENTITY={}", signingIdentity));
	}

	bool result = Commands::subprocess(cmd);

	return result;
}

/*****************************************************************************/
bool CompileStrategyXcodeBuild::buildProject(const SourceTarget& inProject)
{
	UNUSED(inProject);
	return true;
}

}
