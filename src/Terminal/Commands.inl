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
	return Commands::subprocess(inCmd, std::string(), PipeOption::StdOut, PipeOption::StdErr, {}, inCleanOutput);
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, std::string inCwd, const bool inCleanOutput)
{
	return Commands::subprocess(inCmd, std::move(inCwd), PipeOption::StdOut, PipeOption::StdErr, {}, inCleanOutput);
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, std::string inCwd, const PipeOption inStdErr, const bool inCleanOutput)
{
	return Commands::subprocess(inCmd, std::move(inCwd), PipeOption::StdOut, inStdErr, {}, inCleanOutput);
}

/*****************************************************************************/
inline bool Commands::subprocess(const StringList& inCmd, std::string inCwd, const PipeOption inStdOut, const PipeOption inStdErr, const bool inCleanOutput)
{
	return Commands::subprocess(inCmd, std::move(inCwd), inStdOut, inStdErr, {}, inCleanOutput);
}

inline bool Commands::subprocess(const StringList& inCmd, const PipeOption inStdErr, const bool inCleanOutput)
{
	return Commands::subprocess(inCmd, std::string(), PipeOption::StdOut, inStdErr, {}, inCleanOutput);
}

/*****************************************************************************/
inline bool Commands::subprocessNoOutput(const StringList& inCmd, const bool inCleanOutput)
{
	return Commands::subprocess(inCmd, std::string(), PipeOption::Close, PipeOption::Close, {}, inCleanOutput);
}
}
