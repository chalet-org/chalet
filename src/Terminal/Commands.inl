/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Commands.hpp"

namespace chalet
{
/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd)
{
	return Commands::subprocess(inCmd, std::string(), nullptr, PipeOption::StdOut, PipeOption::StdErr, {});
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, CreateSubprocessFunc inOnCreate)
{
	return Commands::subprocess(inCmd, std::string(), std::move(inOnCreate), PipeOption::StdOut, PipeOption::StdErr, {});
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, std::string inCwd)
{
	return Commands::subprocess(inCmd, std::move(inCwd), nullptr, PipeOption::StdOut, PipeOption::StdErr, {});
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, std::string inCwd, const PipeOption inStdErr)
{
	return Commands::subprocess(inCmd, std::move(inCwd), nullptr, PipeOption::StdOut, inStdErr, {});
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, std::string inCwd, const PipeOption inStdOut, const PipeOption inStdErr)
{
	return Commands::subprocess(inCmd, std::move(inCwd), nullptr, inStdOut, inStdErr, {});
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, const PipeOption inStdErr)
{
	return Commands::subprocess(inCmd, std::string(), nullptr, PipeOption::StdOut, inStdErr, {});
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, const PipeOption inStdOut, const PipeOption inStdErr)
{
	return Commands::subprocess(inCmd, std::string(), nullptr, inStdOut, inStdErr, {});
}

/*****************************************************************************/
inline bool Commands::subprocessNoOutput(const StringList& inCmd)
{
	return Commands::subprocess(inCmd, std::string(), nullptr, PipeOption::Close, PipeOption::Close, {});
}

/*****************************************************************************/
inline bool Commands::subprocessNoOutput(const StringList& inCmd, std::string inCwd)
{
	return Commands::subprocess(inCmd, std::move(inCwd), nullptr, PipeOption::Close, PipeOption::Close, {});
}

/*****************************************************************************/
inline bool Commands::subprocessOutputToFile(const StringList& inCmd, const std::string& inOutputFile)
{
	return Commands::subprocessOutputToFile(inCmd, inOutputFile, PipeOption::Pipe);
}
}
