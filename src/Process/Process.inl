/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/Process.hpp"

namespace chalet
{
/*****************************************************************************/
inline bool Process::run(const StringList& inCmd)
{
	return Process::run(inCmd, std::string(), nullptr, PipeOption::StdOut, PipeOption::StdErr);
}

/*****************************************************************************/
inline bool Process::run(const StringList& inCmd, CreateSubprocessFunc inOnCreate)
{
	return Process::run(inCmd, std::string(), std::move(inOnCreate), PipeOption::StdOut, PipeOption::StdErr);
}

/*****************************************************************************/
inline bool Process::run(const StringList& inCmd, std::string inCwd)
{
	return Process::run(inCmd, std::move(inCwd), nullptr, PipeOption::StdOut, PipeOption::StdErr);
}

/*****************************************************************************/
inline bool Process::run(const StringList& inCmd, std::string inCwd, const PipeOption inStdErr)
{
	return Process::run(inCmd, std::move(inCwd), nullptr, PipeOption::StdOut, inStdErr);
}

/*****************************************************************************/
inline bool Process::run(const StringList& inCmd, std::string inCwd, const PipeOption inStdOut, const PipeOption inStdErr)
{
	return Process::run(inCmd, std::move(inCwd), nullptr, inStdOut, inStdErr);
}

/*****************************************************************************/
inline bool Process::run(const StringList& inCmd, const PipeOption inStdErr)
{
	return Process::run(inCmd, std::string(), nullptr, PipeOption::StdOut, inStdErr);
}

/*****************************************************************************/
inline bool Process::run(const StringList& inCmd, const PipeOption inStdOut, const PipeOption inStdErr)
{
	return Process::run(inCmd, std::string(), nullptr, inStdOut, inStdErr);
}

/*****************************************************************************/
inline bool Process::runWithInput(const StringList& inCmd)
{
	return Process::runWithInput(inCmd, std::string(), nullptr, PipeOption::StdOut, PipeOption::StdErr);
}

/*****************************************************************************/
inline bool Process::runWithInput(const StringList& inCmd, CreateSubprocessFunc inOnCreate)
{
	return Process::runWithInput(inCmd, std::string(), std::move(inOnCreate), PipeOption::StdOut, PipeOption::StdErr);
}

/*****************************************************************************/
inline bool Process::runOutputToFile(const StringList& inCmd, const std::string& inOutputFile)
{
	return Process::runOutputToFile(inCmd, inOutputFile, PipeOption::Pipe);
}
}
