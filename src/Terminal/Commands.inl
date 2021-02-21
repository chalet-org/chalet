/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Commands.hpp"

namespace chalet
{
/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, const bool inCleanOutput)
{
	return Commands::subprocess(inCmd, std::string(), nullptr, PipeOption::StdOut, PipeOption::StdErr, {}, inCleanOutput);
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, CreateSubprocessFunc inOnCreate, const bool inCleanOutput)
{
	return Commands::subprocess(inCmd, std::string(), std::move(inOnCreate), PipeOption::StdOut, PipeOption::StdErr, {}, inCleanOutput);
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, std::string inCwd, const bool inCleanOutput)
{
	return Commands::subprocess(inCmd, std::move(inCwd), nullptr, PipeOption::StdOut, PipeOption::StdErr, {}, inCleanOutput);
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, std::string inCwd, const PipeOption inStdErr, const bool inCleanOutput)
{
	return Commands::subprocess(inCmd, std::move(inCwd), nullptr, PipeOption::StdOut, inStdErr, {}, inCleanOutput);
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, std::string inCwd, const PipeOption inStdOut, const PipeOption inStdErr, const bool inCleanOutput)
{
	return Commands::subprocess(inCmd, std::move(inCwd), nullptr, inStdOut, inStdErr, {}, inCleanOutput);
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, const PipeOption inStdErr, const bool inCleanOutput)
{
	return Commands::subprocess(inCmd, std::string(), nullptr, PipeOption::StdOut, inStdErr, {}, inCleanOutput);
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, const PipeOption inStdOut, const PipeOption inStdErr, const bool inCleanOutput)
{
	return Commands::subprocess(inCmd, std::string(), nullptr, inStdOut, inStdErr, {}, inCleanOutput);
}

/*****************************************************************************/
inline bool Commands::subprocessNoOutput(const StringList& inCmd, const bool inCleanOutput)
{
	return Commands::subprocess(inCmd, std::string(), nullptr, PipeOption::Close, PipeOption::Close, {}, inCleanOutput);
}

/*****************************************************************************/
inline bool Commands::subprocessNoOutput(const StringList& inCmd, std::string inCwd, const bool inCleanOutput)
{
	return Commands::subprocess(inCmd, std::move(inCwd), nullptr, PipeOption::Close, PipeOption::Close, {}, inCleanOutput);
}

/*****************************************************************************/
inline bool Commands::subprocessOutputToFile(const StringList& inCmd, const std::string& inOutputFile, const bool inCleanOutput)
{
	return Commands::subprocessOutputToFile(inCmd, inOutputFile, PipeOption::Pipe, inCleanOutput);
}
}
