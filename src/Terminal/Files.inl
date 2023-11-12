/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Files.hpp"

namespace chalet
{
/*****************************************************************************/
inline bool Files::subprocess(const StringList& inCmd)
{
	return Files::subprocess(inCmd, std::string(), nullptr, PipeOption::StdOut, PipeOption::StdErr);
}

/*****************************************************************************/
inline bool Files::subprocess(const StringList& inCmd, CreateSubprocessFunc inOnCreate)
{
	return Files::subprocess(inCmd, std::string(), std::move(inOnCreate), PipeOption::StdOut, PipeOption::StdErr);
}

/*****************************************************************************/
inline bool Files::subprocess(const StringList& inCmd, std::string inCwd)
{
	return Files::subprocess(inCmd, std::move(inCwd), nullptr, PipeOption::StdOut, PipeOption::StdErr);
}

/*****************************************************************************/
inline bool Files::subprocess(const StringList& inCmd, std::string inCwd, const PipeOption inStdErr)
{
	return Files::subprocess(inCmd, std::move(inCwd), nullptr, PipeOption::StdOut, inStdErr);
}

/*****************************************************************************/
inline bool Files::subprocess(const StringList& inCmd, std::string inCwd, const PipeOption inStdOut, const PipeOption inStdErr)
{
	return Files::subprocess(inCmd, std::move(inCwd), nullptr, inStdOut, inStdErr);
}

/*****************************************************************************/
inline bool Files::subprocess(const StringList& inCmd, const PipeOption inStdErr)
{
	return Files::subprocess(inCmd, std::string(), nullptr, PipeOption::StdOut, inStdErr);
}

/*****************************************************************************/
inline bool Files::subprocess(const StringList& inCmd, const PipeOption inStdOut, const PipeOption inStdErr)
{
	return Files::subprocess(inCmd, std::string(), nullptr, inStdOut, inStdErr);
}

/*****************************************************************************/
inline bool Files::subprocessWithInput(const StringList& inCmd)
{
	return Files::subprocessWithInput(inCmd, std::string(), nullptr, PipeOption::StdOut, PipeOption::StdErr);
}

/*****************************************************************************/
inline bool Files::subprocessWithInput(const StringList& inCmd, CreateSubprocessFunc inOnCreate)
{
	return Files::subprocessWithInput(inCmd, std::string(), std::move(inOnCreate), PipeOption::StdOut, PipeOption::StdErr);
}

/*****************************************************************************/
inline bool Files::subprocessOutputToFile(const StringList& inCmd, const std::string& inOutputFile)
{
	return Files::subprocessOutputToFile(inCmd, inOutputFile, PipeOption::Pipe);
}
}
