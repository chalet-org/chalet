/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BuildManager/CmakeBuilder.hpp"

#include "Libraries/Format.hpp"
#include "State/BuildState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CmakeBuilder::CmakeBuilder(const BuildState& inState, const CMakeTarget& inTarget, const bool inCleanOutput) :
	m_state(inState),
	m_target(inTarget),
	m_cleanOutput(inCleanOutput)
{
	UNUSED(m_cleanOutput);
}

/*****************************************************************************/
bool CmakeBuilder::run()
{
	const auto& name = m_target.name();
	const auto& buildConfiguration = m_state.info.buildConfiguration();

	// TODO: add doxygen to path?

	Output::msgBuild(buildConfiguration, name);
	Output::lineBreak();

	auto cwd = Commands::getWorkingDirectory();
	auto location = fmt::format("{}/{}", cwd, m_target.location());
	Path::sanitize(location);

	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	m_outputLocation = fmt::format("{}/{}/{}", cwd, buildOutputDir, m_target.location());
	Path::sanitize(m_outputLocation);

	bool outDirectoryDoesNotExist = !Commands::pathExists(m_outputLocation);
	bool recheckCmake = m_target.recheck();

	if (outDirectoryDoesNotExist || recheckCmake)
	{

		if (outDirectoryDoesNotExist)
			Commands::makeDirectory(m_outputLocation, m_cleanOutput);

		{
			StringList generatorCommand = getGeneratorCommand(location);
			if (!Commands::subprocess(generatorCommand, m_cleanOutput))
				return false;
		}

		{
			StringList buildCommand = getBuildCommand(m_outputLocation);
			if (!Commands::subprocess(buildCommand, PipeOption::StdOut, m_cleanOutput))
				return false;
		}

		Output::lineBreak();
	}

	//
	Output::msgTargetUpToDate(m_state.targets.size() > 1, name);

	return true;
}

/*****************************************************************************/
std::string CmakeBuilder::getGenerator() const
{
	const bool isNinja = m_state.environment.strategy() == StrategyType::Ninja;
	const auto& compileConfig = m_state.compilerTools.getConfig(CodeLanguage::CPlusPlus);

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
	const auto& compileConfig = m_state.compilerTools.getConfig(CodeLanguage::CPlusPlus);

	std::string ret;

	if (!isNinja && compileConfig.isMsvc())
	{
		switch (m_state.info.targetArchitecture())
		{
			case Arch::Cpu::X86:
				ret = "Win32";
				break;
			case Arch::Cpu::X64:
				ret = "x64";
				break;
			case Arch::Cpu::ARM:
				ret = "ARM";
				break;
			case Arch::Cpu::ARM64:
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
	std::string buildConfiguration = m_state.info.buildConfiguration();
	if (m_state.configuration.enableProfiling())
	{
		buildConfiguration = "Debug";
	}
	auto& cmake = m_state.tools.cmake();

	StringList ret{ cmake, "-G", getGenerator() };

	std::string arch = getArch();
	if (!arch.empty())
	{
		ret.push_back("-A");
		ret.push_back(arch);
	}

	const auto& buildScript = m_target.buildScript();
	if (!buildScript.empty())
	{
		ret.push_back("-C");
		ret.push_back(buildScript);
	}

	const auto& toolset = m_target.toolset();
	if (!toolset.empty())
	{
		ret.push_back("-T");
		ret.push_back(toolset);
	}

	for (auto& define : m_target.defines())
	{
		ret.push_back("-D" + define);
	}
	ret.push_back("-DCMAKE_BUILD_TYPE=" + buildConfiguration);

	ret.push_back("-S");
	ret.push_back(inLocation);

	ret.push_back("-B");
	ret.push_back(m_outputLocation);

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
