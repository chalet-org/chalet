/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Process/PipeOption.hpp"

namespace chalet
{
namespace Process
{
using CreateSubprocessFunc = std::function<void(i32 /* pid */)>;

inline bool run(const StringList& inCmd);
inline bool run(const StringList& inCmd, CreateSubprocessFunc inOnCreate);
inline bool run(const StringList& inCmd, std::string inCwd);
inline bool run(const StringList& inCmd, std::string inCwd, const PipeOption inStdErr);
inline bool run(const StringList& inCmd, std::string inCwd, const PipeOption inStdOut, const PipeOption inStdErr);
inline bool run(const StringList& inCmd, const PipeOption inStdOut);
inline bool run(const StringList& inCmd, const PipeOption inStdOut, const PipeOption inStdErr);
inline bool runWithInput(const StringList& inCmd);
inline bool runWithInput(const StringList& inCmd, CreateSubprocessFunc inOnCreate);
inline bool runOutputToFile(const StringList& inCmd, const std::string& inOutputFile);

bool run(const StringList& inCmd, std::string inCwd, CreateSubprocessFunc inOnCreate, const PipeOption inStdOut, const PipeOption inStdErr);
bool runWithInput(const StringList& inCmd, std::string inCwd, CreateSubprocessFunc inOnCreate, const PipeOption inStdOut, const PipeOption inStdErr);
bool runNoOutput(const StringList& inCmd);
bool runMinimalOutput(const StringList& inCmd);
bool runMinimalOutput(const StringList& inCmd, std::string inCwd);
bool runOutputToFile(const StringList& inCmd, const std::string& inOutputFile, const PipeOption inStdErr);
std::string runOutput(const StringList& inCmd, const PipeOption inStdOut = PipeOption::Pipe, const PipeOption inStdErr = PipeOption::Pipe);
std::string runOutput(const StringList& inCmd, std::string inWorkingDirectory, const PipeOption inStdOut = PipeOption::Pipe, const PipeOption inStdErr = PipeOption::Pipe);

bool runNinjaBuild(const StringList& inCmd, std::string inCwd = std::string());

}
}

#include "Process/Process.inl"
