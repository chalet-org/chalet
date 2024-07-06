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
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "Process/ProcessOptions.hpp"
#include "Process/SubProcessController.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonValues.hpp"

namespace chalet
{
/*****************************************************************************/
CompileStrategyMSBuild::CompileStrategyMSBuild(BuildState& inState) :
	ICompileStrategy(StrategyType::MSBuild, inState)
{
}

/*****************************************************************************/
std::string CompileStrategyMSBuild::name() const noexcept
{
	return "MSBuild";
}

/*****************************************************************************/
bool CompileStrategyMSBuild::initialize()
{
	if (m_initialized)
		return false;

	// auto& cacheFile = m_state.cache.file();
	const auto& cachePathId = m_state.cachePathId();
	UNUSED(m_state.cache.getCachePath(cachePathId));

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

	auto& cwd = m_state.inputs.workingDirectory();

	auto msbuild = Files::which("msbuild");
	if (msbuild.empty())
	{
		Diagnostic::error("MSBuild is required, but was not found in path.");
		return false;
	}

	VSSolutionProjectExporter exporter(m_state.inputs);
	m_solution = exporter.getMainProjectOutput(m_state);

	// TODO: In a recent version of MSBuild (observed in 17.6.3),
	//   there's an extra line break in minimal verbosity mode.
	//   Unsure if it's intentional, or a bug, but we'll handle it for now
	//
	// if (m_state.toolchain.versionMajorMinor() >= 1706 && !Output::showCommands())
	// 	Output::previousLine();

	auto buildTargets = m_state.inputs.getBuildTargets();
	if (buildTargets.empty())
		buildTargets.emplace_back(Values::All);

	if (!Output::showCommands())
		Environment::set("BUILD_FROM_CHALET", "1");

	const bool keepGoing = m_state.info.keepGoing();
	bool result = !buildTargets.empty();
	for (auto& target : buildTargets)
	{
		auto cmd = getMsBuildCommand(msbuild, target);
		if (Output::showCommands())
		{
			result &= Process::run(cmd);
		}
		else
		{
			result &= subprocessMsBuild(cmd, cwd);
		}

		if (!keepGoing && !result)
			break;
	}

	if (result)
	{
		auto sln = m_solution;
		String::replaceAll(sln, fmt::format("{}/", cwd), "");
		Output::msgAction("Succeeded", sln);
	}

	if (!Output::showCommands())
		Environment::set("BUILD_FROM_CHALET", std::string());

	if (!result)
	{
		m_filesUpdated = true;
	}
	else
	{
		for (auto& target : m_state.targets)
		{
			if (target->isSources())
				checkIfTargetWasUpdated(static_cast<const SourceTarget&>(*target));
		}
	}

	return result;
}

/*****************************************************************************/
bool CompileStrategyMSBuild::buildProject(const SourceTarget& inProject)
{
	UNUSED(inProject);
	return true;
}

/*****************************************************************************/
StringList CompileStrategyMSBuild::getMsBuildCommand(const std::string& msbuild, const std::string& inProjectName) const
{
	chalet_assert(!m_solution.empty(), "m_solution was not assigned");

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

	cmd.emplace_back(fmt::format("-target:{}", getMsBuildTarget()));

	if (String::equals(Values::All, inProjectName) || !m_state.info.onlyRequired())
	{
		cmd.emplace_back(m_solution);
	}
	else
	{
		auto folder = String::getPathFolder(m_solution);
		cmd.emplace_back(fmt::format("{}/vcxproj/{}.vcxproj", folder, inProjectName));
	}

	return cmd;
}

/*****************************************************************************/
std::string CompileStrategyMSBuild::getMsBuildTarget() const
{
	auto& route = m_state.inputs.route();

	if (route.isClean())
		return "Clean";
	else if (route.isRebuild())
		return "Clean,Build";
	else
		return "Build";
}

/*****************************************************************************/
bool CompileStrategyMSBuild::subprocessMsBuild(const StringList& inCmd, std::string inCwd) const
{
	if (Output::showCommands())
		Output::printCommand(inCmd);

	const auto& color = Output::getAnsiStyle(Output::theme().build);
	const auto& reset = Output::getAnsiStyle(Output::theme().reset);

	auto cwd = inCwd;
	Path::toUnix(cwd);
	cwd += '/';

	bool allowOutput = false;
	std::string errors;
	auto processLine = [&cwd, &allowOutput, &color, &reset, &errors](std::string& inLine) {
		if (!inLine.empty())
		{
			if (String::startsWith("\x1b[m", inLine))
				inLine = inLine.substr(3);

			if (String::startsWith("  ", inLine))
				inLine = inLine.substr(2);

			if (String::startsWith("*== script start ==*", inLine))
			{
				inLine.clear();
				allowOutput = true;
			}
			else if (String::startsWith("*== script end ==*", inLine))
			{
				allowOutput = false;
			}
			else if (String::contains(": error ", inLine))
			{
				errors += inLine;
			}
			else if (String::contains(": warning ", inLine))
			{
				std::cout.write(inLine.data(), inLine.size());
				std::cout.flush();
			}
			else if (String::startsWith("   Creating library", inLine))
			{
				Path::toUnix(inLine);
				String::replaceAll(inLine, cwd, "");
				auto coloredLine = fmt::format("{}{}{}", color, inLine, reset);
				std::cout.write(coloredLine.data(), coloredLine.size());
				std::cout.flush();
			}
			else if (!allowOutput)
			{
				Path::toUnix(inLine);
				String::replaceAll(inLine, cwd, "");
				auto coloredLine = fmt::format("   {}{}{}", color, inLine, reset);
				std::cout.write(coloredLine.data(), coloredLine.size());
				std::cout.flush();
			}
			if (allowOutput && !inLine.empty())
			{
				std::cout.write(inLine.data(), inLine.size());
				std::cout.flush();
			}
		}
	};

	std::string data;
	std::string eol = String::eol();

	ProcessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::Pipe;
	options.onStdErr = [&errors](std::string inData) -> void {
		errors += std::move(inData);
	};
	options.onStdOut = [&processLine, &data, &eol](std::string inData) -> void {
		auto lineBreak = inData.find(eol);
		if (lineBreak == std::string::npos)
		{
			data += std::move(inData);
		}
		else
		{
			while (lineBreak != std::string::npos)
			{
				if (lineBreak > 0)
				{
					data += inData.substr(0, lineBreak + eol.size());
				}
				processLine(data);
				data.clear();

				inData = inData.substr(lineBreak + eol.size());
				lineBreak = inData.find(eol);
				if (lineBreak == std::string::npos)
				{
					data += std::move(inData);
				}
			}
		}
	};

	i32 result = SubProcessController::run(inCmd, options);

	if (!errors.empty())
	{
		std::cout.write(errors.data(), errors.size());
	}

	return result == EXIT_SUCCESS;
}

}
