/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/CmakeBuilder.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Process/Process.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CmakeBuilder::CmakeBuilder(const BuildState& inState, const CMakeTarget& inTarget) :
	m_state(inState),
	m_target(inTarget)
{
}

/*****************************************************************************/
bool CmakeBuilder::run()
{
	const auto& name = m_target.name();

	// TODO: add doxygen to path?

	Output::msgBuild(name);
	Output::lineBreak();

	auto location = Commands::getAbsolutePath(m_target.location());
	Path::sanitize(location);

	if (!m_target.buildFile().empty())
	{
		m_buildFile = fmt::format("{}/{}", location, m_target.buildFile());
	}

	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	m_outputLocation = fmt::format("{}/{}", Commands::getAbsolutePath(buildOutputDir), m_target.location());
	Path::sanitize(m_outputLocation);

	auto onRunFailure = [this]() -> bool {
		Commands::removeRecursively(m_outputLocation);
		return false;
	};

	bool outDirectoryDoesNotExist = !Commands::pathExists(m_outputLocation);
	bool recheckCmake = m_target.recheck();

	if (outDirectoryDoesNotExist || recheckCmake)
	{
		if (outDirectoryDoesNotExist)
			Commands::makeDirectory(m_outputLocation);

		{
			StringList generatorCommand = getGeneratorCommand(location);

#if defined(CHALET_WIN32)
			if (Environment::isBash())
			{
				ProcessOptions options;
				options.stderrOption = PipeOption::StdErr;
				options.stdoutOption = PipeOption::Pipe;
				options.onStdOut = [](std::string inData) {
					String::replaceAll(inData, "\r\n", "\n");
					std::cout << std::move(inData) << std::flush;
				};
				if (Process::run(generatorCommand, options) != EXIT_SUCCESS)
					return onRunFailure();
			}
			else
#endif
			{
				if (!Commands::subprocess(generatorCommand))
					return onRunFailure();
			}
		}

		{
			StringList buildCommand = getBuildCommand(m_outputLocation);
			if (!Commands::subprocess(buildCommand, PipeOption::StdOut))
				return onRunFailure();
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
	const bool isNinja = m_state.toolchain.strategy() == StrategyType::Ninja;

	std::string ret;
	if (isNinja)
	{
		ret = "Ninja";
	}
#if defined(CHALET_WIN32)
	else if (m_state.environment->isMsvc())
	{
		// Validated in CMakeTarget::validate
		const auto& version = m_state.toolchain.version();
		if (String::startsWith("17.", version))
		{
			ret = "Visual Studio 17 2022";
		}
		else if (String::startsWith("16.", version))
		{
			ret = "Visual Studio 16 2019";
		}
		else if (String::startsWith("15.", version))
		{
			ret = "Visual Studio 15 2017";
		}
		else if (String::startsWith("14.", version))
		{
			ret = "Visual Studio 14 2015";
		}
		else if (String::startsWith("12.", version))
		{
			ret = "Visual Studio 12 2013";
		}
		else if (String::startsWith("11.", version))
		{
			ret = "Visual Studio 11 2012";
		}
		else if (String::startsWith("10.", version))
		{
			ret = "Visual Studio 10 2010";
		}
	}
#endif
	else
	{
#if defined(CHALET_WIN32)
		ret = "MinGW Makefiles";
#else
		ret = "Unix Makefiles";
#endif
	}

	return ret;
}

/*****************************************************************************/
std::string CmakeBuilder::getArchitecture() const
{
	const bool isNinja = m_state.toolchain.strategy() == StrategyType::Ninja;

	std::string ret;

	// Note: The -A flag is only really used by VS
	if (!isNinja && m_state.environment->isMsvc())
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
	auto& cmake = m_state.toolchain.cmake();

	auto generator = getGenerator();
	chalet_assert(!generator.empty(), "CMake Generator is empty");

	StringList ret{ cmake, "-G", std::move(generator) };

	std::string arch = getArchitecture();
	if (!arch.empty())
	{
		ret.emplace_back("-A");
		ret.emplace_back(std::move(arch));
	}

	if (!m_buildFile.empty())
	{
		ret.emplace_back("-C");
		ret.push_back(m_buildFile);
	}

	const auto& toolset = m_target.toolset();
	if (!toolset.empty())
	{
		ret.emplace_back("-T");
		ret.push_back(toolset);
	}

	addCmakeDefines(ret);

	ret.emplace_back("-S");
	ret.push_back(inLocation);

	ret.emplace_back("-B");
	ret.push_back(m_outputLocation);

	return ret;
}

/*****************************************************************************/
void CmakeBuilder::addCmakeDefines(StringList& outList) const
{
	struct charCompare
	{
		bool operator()(const char* inA, const char* inB) const
		{
			return std::strcmp(inA, inB) < 0;
		}
	};

	std::map<const char*, bool, charCompare> isDefined;
	for (auto& define : m_target.defines())
	{
		outList.emplace_back("-D" + define);

		if (String::contains("CMAKE_EXPORT_COMPILE_COMMANDS", define))
			isDefined["CMAKE_EXPORT_COMPILE_COMMANDS"] = true;
		else if (String::contains("CMAKE_C_COMPILER", define))
			isDefined["CMAKE_C_COMPILER"] = true;
		else if (String::contains("CMAKE_CXX_COMPILER", define))
			isDefined["CMAKE_CXX_COMPILER"] = true;
		else if (String::contains("CMAKE_BUILD_TYPE", define))
			isDefined["CMAKE_BUILD_TYPE"] = true;
#if defined(CHALET_WIN32)
		else if (String::contains("CMAKE_SH", define))
			isDefined["CMAKE_SH"] = true;
#elif defined(CHALET_MACOS)
		else if (String::contains("CMAKE_OSX_ARCHITECTURES", define))
			isDefined["CMAKE_OSX_ARCHITECTURES"] = true;
#endif
	}

	if (m_state.info.generateCompileCommands())
	{
		if (!isDefined["EXPORT_COMPILE_COMMANDS"])
		{
			outList.emplace_back("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON");
		}
	}

	if (!isDefined["CMAKE_C_COMPILER"])
	{
		const auto& compilerC = m_state.toolchain.compilerC().path;
		outList.emplace_back(fmt::format("-DCMAKE_C_COMPILER={}", compilerC));
	}

	if (!isDefined["CMAKE_CXX_COMPILER"])
	{
		const auto& compilerC = m_state.toolchain.compilerCpp().path;
		outList.emplace_back(fmt::format("-DCMAKE_CXX_COMPILER={}", compilerC));
	}

	if (!isDefined["CMAKE_BUILD_TYPE"])
	{
		std::string buildConfiguration = getCMakeCompatibleBuildConfiguration();
		outList.emplace_back("-DCMAKE_BUILD_TYPE=" + buildConfiguration);
	}

#if defined(CHALET_WIN32)
	if (!isDefined["CMAKE_SH"])
	{
		if (Commands::which("sh").empty())
			outList.emplace_back("-DCMAKE_SH=\"CMAKE_SH-NOTFOUND\"");
	}
#elif defined(CHALET_MACOS)
	if (!isDefined["CMAKE_OSX_ARCHITECTURES"])
	{
		if (!m_state.info.universalArches().empty())
		{
			auto value = String::join(m_state.info.universalArches(), ';');
			outList.emplace_back("-DCMAKE_OSX_ARCHITECTURES=" + value);
		}
		else
		{
			const auto& targetArch = m_state.info.targetArchitectureString();
			outList.emplace_back("-DCMAKE_OSX_ARCHITECTURES=" + targetArch);
		}
	}

#endif
}

/*****************************************************************************/
std::string CmakeBuilder::getCMakeCompatibleBuildConfiguration() const
{
	std::string ret = m_state.info.buildConfiguration();

	if (m_state.configuration.isMinSizeRelease())
	{
		ret = "MinSizeRel";
	}
	else if (m_state.configuration.isReleaseWithDebugInfo())
	{
		ret = "RelWithDebInfo";
	}
	else if (m_state.configuration.isDebuggable())
	{
		// Profile > Debug in CMake
		ret = "Debug";
	}
	else
	{
		// RelStable > Release in CMake
		ret = "Release";
	}

	return ret;
}

/*****************************************************************************/
StringList CmakeBuilder::getBuildCommand(const std::string& inLocation) const
{
	auto& cmake = m_state.toolchain.cmake();
	const auto maxJobs = m_state.info.maxJobs();
	const bool isMake = m_state.toolchain.strategy() == StrategyType::Makefile;

	StringList ret{ cmake, "--build", inLocation, "-j", std::to_string(maxJobs) };

	if (isMake)
	{
		ret.emplace_back("--");
		if (m_state.toolchain.makeVersionMajor() >= 4)
		{
			ret.emplace_back("--output-sync=target");
		}
		ret.emplace_back("--no-builtin-rules");
		ret.emplace_back("--no-builtin-variables");
		ret.emplace_back("--no-print-directory");
	}

	// LOG(String::join(ret));

	return ret;
}

}
