/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BuildManager/CmakeBuilder.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CmakeBuilder::CmakeBuilder(const BuildState& inState, const ProjectConfiguration& inProject, const bool inCleanOutput) :
	m_state(inState),
	m_project(inProject),
	m_cleanOutput(inCleanOutput)
{
	UNUSED(m_cleanOutput);
}

/*****************************************************************************/
bool CmakeBuilder::run()
{
	// TODO: check for cmake executable
	// TODO: add doxygen to path?

	const auto& buildConfiguration = m_state.buildConfiguration();
	const auto& name = m_project.name();

	Output::msgBuild(buildConfiguration, name);
	Output::lineBreak();

	std::string cmakeBuild = buildConfiguration;
	if (cmakeBuild != "Release" && cmakeBuild != "Debug" && cmakeBuild != "RelWithDebInfo" && cmakeBuild != "MinSizeRel")
	{
		// https://cmake.org/cmake/help/v3.0/variable/CMAKE_BUILD_TYPE.html
		Diagnostic::error(fmt::format("Build '{}' not recognized by CMAKE.", buildConfiguration));
		return false;
	}

	const auto& loc = m_project.locations().front();
	const auto& buildOutputDir = m_state.paths.buildOutputDir();

	auto cwd = Commands::getWorkingDirectoryPath();
	auto outDirPath = cwd / loc / buildOutputDir;
	std::string outDir = outDirPath.string();

	Path::sanitize(outDir);

	bool outDirectoryDoesNotExist = !Commands::pathExists(outDir);
	bool recheckCmake = m_project.cmakeRecheck();

	if (outDirectoryDoesNotExist || recheckCmake)
	{
		auto locationPath = cwd / loc;
		std::string location = locationPath.string();

		Path::sanitize(location);

		if (outDirectoryDoesNotExist)
			Commands::makeDirectory(outDir, false);

		std::string defines = String::getPrefixed(m_project.cmakeDefines(), "-D");

		const bool ninja = m_state.environment.strategy() == StrategyType::Ninja;
		const auto& compileConfig = m_state.compilers.getConfig(m_project.language());

		std::string generator;
		if (ninja)
			generator = "Ninja";
		else if (compileConfig.isMingw())
			generator = "MinGW Makefiles";
		else
			generator = "Unix Makefiles";

		// TODO: -A arch, -T toolset

		std::string cmakeCommand = fmt::format("cd {outDir} && {cmake} -G \"{generator}\" {location} {defines} -DCMAKE_BUILD_TYPE={cmakeBuild}",
			FMT_ARG(outDir),
			fmt::arg("cmake", m_state.tools.cmake()),
			FMT_ARG(generator),
			FMT_ARG(location),
			FMT_ARG(defines),
			FMT_ARG(cmakeBuild));

		if (!Commands::shell(cmakeCommand))
			return false;

		if (ninja)
		{
			const auto& ninjaExec = m_state.tools.ninja();

			if (!Commands::subprocess({ ninjaExec }, true, PipeOption::StdOut, PipeOption::StdOut, {}, outDir))
				return false;
		}
		else
		{
			const auto& makeExec = m_state.tools.make();

			const auto maxJobs = m_state.environment.maxJobs();
			std::string jobs;
			if (maxJobs > 0)
				jobs = fmt::format(" -j{}", maxJobs);

			std::string syncTarget;
			if (m_state.tools.makeVersionMajor() >= 4)
				syncTarget = " --output-sync=target";

			auto makeCommand = fmt::format("cd {outDir} && {makeExec}{jobs}{syncTarget}",
				FMT_ARG(outDir),
				FMT_ARG(makeExec),
				FMT_ARG(jobs),
				FMT_ARG(syncTarget));

			if (!Commands::shell(makeCommand))
				return false;
		}

		Output::lineBreak();
	}

	//
	Output::msgTargetUpToDate(m_state.projects.size() > 1, m_project.name());

	return true;
}

}
