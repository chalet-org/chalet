/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BuildManager/CmakeBuilder.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
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
	const auto& name = m_project.name();
	if (!m_state.tools.cmakeAvailable())
	{
		Diagnostic::error(fmt::format("CMake was requsted for the project '{}' but was not found.", name));
		return false;
	}

	// TODO: add doxygen to path?

	const auto& buildConfiguration = m_state.buildConfiguration();

	Output::msgBuild(buildConfiguration, name);
	Output::lineBreak();

	if (!List::contains({ "Release", "Debug", "RelWithDebInfo", "MinSizeRel" }, buildConfiguration))
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

		const bool isNinja = m_state.environment.strategy() == StrategyType::Ninja;

		// TODO: -A arch, -T toolset

		StringList cmakeCommand = getCmakeCommand(location);

		if (!Commands::subprocess(cmakeCommand, outDir))
			return false;

		if (isNinja)
		{
			const auto& ninjaExec = m_state.tools.ninja();
			if (!Commands::subprocess({ ninjaExec }, outDir, PipeOption::StdOut))
				return false;
		}
		else
		{
			const auto& makeExec = m_state.tools.make();
			const auto maxJobs = m_state.environment.maxJobs();

			StringList makeCommand{ makeExec };

			if (maxJobs > 0)
				makeCommand.push_back(fmt::format("-j{}", maxJobs));

			if (m_state.tools.makeVersionMajor() >= 4)
				makeCommand.push_back("--output-sync=target");

			if (!Commands::subprocess(makeCommand, outDir))
				return false;
		}

		Output::lineBreak();
	}

	//
	Output::msgTargetUpToDate(m_state.projects.size() > 1, m_project.name());

	return true;
}

/*****************************************************************************/
std::string CmakeBuilder::getGenerator() const
{
	const bool isNinja = m_state.environment.strategy() == StrategyType::Ninja;
	const auto& compileConfig = m_state.compilers.getConfig(m_project.language());

	std::string ret;
	if (isNinja)
		ret = "Ninja";
	else if (compileConfig.isMingw())
		ret = "MinGW Makefiles";
	else
		ret = "Unix Makefiles";

	return ret;
}

/*****************************************************************************/
StringList CmakeBuilder::getCmakeCommand(const std::string& inLocation) const
{
	const auto& buildConfiguration = m_state.buildConfiguration();
	auto& cmake = m_state.tools.cmake();

	StringList ret{ cmake, "-G", getGenerator(), inLocation };
	for (auto& define : m_project.cmakeDefines())
	{
		ret.push_back("-D" + define);
	}
	ret.push_back("-DCMAKE_BUILD_TYPE=" + buildConfiguration);

	return ret;
}
}
