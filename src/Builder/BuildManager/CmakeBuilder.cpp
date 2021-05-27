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

	// TODO: This should come from a single location
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

		// TODO: -T toolset (relates to Host arch)
		// https://cmake.org/cmake/help/latest/variable/CMAKE_GENERATOR_TOOLSET.html#variable:CMAKE_GENERATOR_TOOLSET

		{
			StringList generatorCommand = getGeneratorCommand(location);
			if (!Commands::subprocess(generatorCommand, outDir))
				return false;
		}

		{
			StringList buildCommand = getBuildCommand(".");
			if (!Commands::subprocess(buildCommand, outDir, PipeOption::StdOut))
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
	const auto& compileConfig = m_state.compilerTools.getConfig(m_project.language());

	std::string ret;
	if (isNinja)
	{
		ret = "Ninja";
	}
	else if (compileConfig.isMsvc())
	{
		ret = "Visual Studio 16 2019";
	}
	else if (compileConfig.isMingw())
	{
		ret = "MinGW Makefiles";
	}
	else
	{
		ret = "Unix Makefiles";
	}

	return ret;
}

/*****************************************************************************/
std::string CmakeBuilder::getArch() const
{
	const bool isNinja = m_state.environment.strategy() == StrategyType::Ninja;
	const auto& compileConfig = m_state.compilerTools.getConfig(m_project.language());

	std::string ret;

	if (!isNinja && compileConfig.isMsvc())
	{
		switch (m_state.targetArchitecture())
		{
			case CpuArchitecture::X86:
				ret = "Win32";
				break;
			case CpuArchitecture::X64:
				ret = "x64";
				break;
			case CpuArchitecture::ARM:
				ret = "ARM";
				break;
			case CpuArchitecture::ARM64:
				ret = "ARM64";
				break;
			default:
				break;
		}
	}

	return ret;
}

/*****************************************************************************/
StringList CmakeBuilder::getGeneratorCommand(const std::string& inLocation) const
{
	const auto& buildConfiguration = m_state.buildConfiguration();
	auto& cmake = m_state.tools.cmake();

	StringList ret{ cmake, "-G", getGenerator() };
	std::string arch = getArch();
	if (!arch.empty())
	{
		ret.push_back("-A");
		ret.push_back(arch);
	}

	ret.push_back(inLocation);
	for (auto& define : m_project.cmakeDefines())
	{
		ret.push_back("-D" + define);
	}
	ret.push_back("-DCMAKE_BUILD_TYPE=" + buildConfiguration);

	return ret;
}

/*****************************************************************************/
StringList CmakeBuilder::getBuildCommand(const std::string& inLocation) const
{
	auto& cmake = m_state.tools.cmake();
	const auto maxJobs = m_state.environment.maxJobs();

	const bool isMake = m_state.environment.strategy() == StrategyType::Makefile;

	StringList ret{ cmake, "--build", inLocation, "-j", std::to_string(maxJobs) };

	if (isMake && m_state.tools.makeVersionMajor() >= 4)
	{
		ret.push_back("--");
		ret.push_back("--output-sync=target");
	}

	LOG(String::join(ret));

	return ret;
}

}
