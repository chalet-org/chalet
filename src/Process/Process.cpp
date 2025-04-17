/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/Process.hpp"

#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "Process/SubProcessController.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
/*****************************************************************************/
void stripLastEndLine(std::string& inString)
{
	if (!inString.empty() && inString.back() == '\n')
	{
		inString.pop_back();
#if defined(CHALET_WIN32)
		if (!inString.empty() && inString.back() == '\r')
			inString.pop_back();
#endif
	}
}
}

/*****************************************************************************/
bool Process::run(const StringList& inCmd, std::string inCwd, CreateSubprocessFunc inOnCreate, const PipeOption inStdOut, const PipeOption inStdErr, const bool inWaitForResult)
{
	if (Output::showCommands())
		Output::printCommand(inCmd);

	chalet_assert(inStdOut != PipeOption::Pipe, "Process::run must implement onStdOut");
	chalet_assert(inStdErr != PipeOption::Pipe, "Process::run must implement onStdErr");

	ProcessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = inStdOut;
	options.stderrOption = inStdErr;
	options.onCreate = std::move(inOnCreate);
	options.waitForResult = inWaitForResult;

	return SubProcessController::run(inCmd, options) == EXIT_SUCCESS;
}

/*****************************************************************************/
bool Process::runWithInput(const StringList& inCmd, std::string inCwd, CreateSubprocessFunc inOnCreate, const PipeOption inStdOut, const PipeOption inStdErr)
{
	if (Output::showCommands())
		Output::printCommand(inCmd);

	chalet_assert(inStdOut != PipeOption::Pipe, "Process::run must implement onStdOut");
	chalet_assert(inStdErr != PipeOption::Pipe, "Process::run must implement onStdErr");

	ProcessOptions options;
	options.cwd = std::move(inCwd);
	options.stdinOption = PipeOption::StdIn;
	options.stdoutOption = inStdOut;
	options.stderrOption = inStdErr;
	options.onCreate = std::move(inOnCreate);

	return SubProcessController::run(inCmd, options) == EXIT_SUCCESS;
}

/*****************************************************************************/
std::string Process::runOutput(const StringList& inCmd, const PipeOption inStdOut, const PipeOption inStdErr)
{
	return Process::runOutput(inCmd, Files::getWorkingDirectory(), inStdOut, inStdErr);
}

/*****************************************************************************/
std::string Process::runOutput(const StringList& inCmd, std::string inWorkingDirectory, const PipeOption inStdOut, const PipeOption inStdErr)
{
	if (Output::showCommands())
		Output::printCommand(inCmd);

	std::string ret;

	ProcessOptions options;
	options.waitForResult = true;
	options.cwd = std::move(inWorkingDirectory);
	options.stdoutOption = inStdOut;
	options.stderrOption = inStdErr;
	if (options.stdoutOption == PipeOption::Pipe)
	{
		options.onStdOut = [&ret](std::string inData) {
#if defined(CHALET_WIN32)
			String::replaceAll(inData, "\r\n", "\n");
#endif
			ret += std::move(inData);
		};
	}
	if (options.stderrOption == PipeOption::Pipe)
	{
		options.onStdErr = [&ret](std::string inData) {
#if defined(CHALET_WIN32)
			String::replaceAll(inData, "\r\n", "\n");
#endif
			ret += std::move(inData);
		};
	}
	else if (options.stderrOption == PipeOption::Close)
	{
		options.stderrOption = PipeOption::Pipe;
		options.onStdErr = [](std::string inData) {
			UNUSED(inData);
		};
	}

	UNUSED(SubProcessController::run(inCmd, options));

	stripLastEndLine(ret);

	return ret;
}

/*****************************************************************************/
bool Process::runNoOutput(const StringList& inCmd)
{
	if (Output::showCommands())
		return Process::run(inCmd, std::string(), nullptr, PipeOption::StdOut, PipeOption::StdErr);
	else
		return Process::run(inCmd, std::string(), nullptr, PipeOption::Close, PipeOption::Close);
}

/*****************************************************************************/
bool Process::runMinimalOutput(const StringList& inCmd)
{
	if (Output::showCommands())
		return Process::run(inCmd, std::string(), nullptr, PipeOption::StdOut, PipeOption::StdErr);
	else
		return Process::run(inCmd, std::string(), nullptr, PipeOption::Close, PipeOption::StdErr);
}

/*****************************************************************************/
bool Process::runMinimalOutput(const StringList& inCmd, std::string inCwd)
{
	if (Output::showCommands())
		return Process::run(inCmd, std::move(inCwd), nullptr, PipeOption::StdOut, PipeOption::StdErr);
	else
		return Process::run(inCmd, std::move(inCwd), nullptr, PipeOption::Close, PipeOption::StdErr);
}

/*****************************************************************************/
bool Process::runMinimalOutputWithoutWait(const StringList& inCmd)
{
	if (Output::showCommands())
		return Process::run(inCmd, std::string(), nullptr, PipeOption::StdOut, PipeOption::StdErr, false);
	else
		return Process::run(inCmd, std::string(), nullptr, PipeOption::Close, PipeOption::StdErr, false);
}

/*****************************************************************************/
bool Process::runMinimalOutputWithoutWait(const StringList& inCmd, std::string inCwd)
{
	if (Output::showCommands())
		return Process::run(inCmd, std::move(inCwd), nullptr, PipeOption::StdOut, PipeOption::StdErr, false);
	else
		return Process::run(inCmd, std::move(inCwd), nullptr, PipeOption::Close, PipeOption::StdErr, false);
}

/*****************************************************************************/
bool Process::runOutputToFile(const StringList& inCmd, const std::string& inOutputFile, const PipeOption inStdErr)
{
	if (Output::showCommands())
		Output::printCommand(inCmd);

	auto outputStream = Files::ofstream(inOutputFile);

	ProcessOptions options;
	options.cwd = Files::getWorkingDirectory();
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = inStdErr;
	options.onStdOut = [&outputStream](std::string inData) {
#if defined(CHALET_WIN32)
		String::replaceAll(inData, "\r\n", "\n");
#endif
		outputStream << inData;
	};
	if (options.stderrOption == PipeOption::Pipe)
	{
		options.onStdErr = options.onStdOut;
	}

	bool result = SubProcessController::run(inCmd, options) == EXIT_SUCCESS;
	outputStream << std::endl;
	return result;
}

/*****************************************************************************/
bool Process::runOutputToFileThroughShell(const StringList& inCmd, const std::string& inOutputFile)
{
	if (inCmd.empty())
		return false;

	StringList shellCmd = inCmd;
	shellCmd[0] = fmt::format("\"{}\"", shellCmd[0]);
	shellCmd.emplace_back(">");
	shellCmd.emplace_back(inOutputFile);
	shellCmd.emplace_back("2>&1");
#if defined(CHALET_WIN32)
	auto& cmd = shellCmd;
#else
	auto shellCmdString = String::join(shellCmd);

	auto bash = Environment::getShell();

	StringList cmd;
	cmd.emplace_back(std::move(bash));
	cmd.emplace_back("-c");
	cmd.emplace_back(fmt::format("'{}'", shellCmdString));

#endif

	auto outCmd = String::join(cmd);
	bool result = std::system(outCmd.c_str()) == EXIT_SUCCESS;
	return result;
}

/*****************************************************************************/
bool Process::runNinjaBuild(const StringList& inCmd, std::string inCwd)
{
	if (Output::showCommands())
		Output::printCommand(inCmd);

	std::string capData;
	static struct
	{
		std::string eol = String::eol();
		std::string endlineReplace = fmt::format("{}\n", Output::getAnsiStyle(Output::theme().reset));
	} cap;

	ProcessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = PipeOption::Pipe;
#if defined(CHALET_WIN32)
	options.stderrOption = PipeOption::StdOut;
#else
	options.stderrOption = PipeOption::StdErr;
#endif
	options.onStdOut = [&capData](std::string inData) -> void {
		String::replaceAll(inData, cap.eol, cap.endlineReplace);
		std::cout.write(inData.data(), inData.size());
		std::cout.flush();

		auto lineBreak = inData.find('\n');
		if (lineBreak == std::string::npos)
		{
			capData += std::move(inData);
		}
		else
		{
			capData += inData.substr(0, lineBreak + 1);
			auto tmp = inData.substr(lineBreak + 1);
			if (!tmp.empty())
			{
				capData = std::move(tmp);
			}
		}
	};

	i32 result = SubProcessController::run(inCmd, options);

	if (!capData.empty())
	{
		std::string noWork = fmt::format("ninja: no work to do.{}", cap.endlineReplace);
		if (String::endsWith(noWork, capData))
			Output::previousLine(true);
	}

	return result == EXIT_SUCCESS;
}
}
