/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMANDS_HPP
#define CHALET_COMMANDS_HPP

#include "Utility/SubprocessTypes.hpp"

namespace chalet
{
namespace Commands
{
using CreateSubprocessFunc = std::function<void(int /* pid */)>;

std::string getWorkingDirectory();
fs::path getWorkingDirectoryPath();
bool changeWorkingDirectory(const std::string& inValue);

bool pathIsFile(const std::string& inValue);
bool pathIsDirectory(const std::string& inValue);

std::string getCanonicalPath(const std::string& inPath);
std::string getAbsolutePath(const std::string& inPath);

std::uintmax_t getPathSize(const std::string& inPath, const bool inCleanOutput = true);
std::int64_t getLastWriteTime(const std::string& inFile);

bool makeDirectory(const std::string& inPath, const bool inCleanOutput = true);
bool makeDirectories(const StringList& inPaths, const bool inCleanOutput = true);
bool remove(const std::string& inPath, const bool inCleanOutput = true);
bool removeRecursively(const std::string& inPath, const bool inCleanOutput = true);
bool setExecutableFlag(const std::string& inPath, const bool inCleanOutput = true);
bool createDirectorySymbolicLink(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput = true);
bool createSymbolicLink(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput = true);
bool copy(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput = true);
bool copySkipExisting(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput = true);
bool copyRename(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput = true);
bool rename(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput = true);
void forEachFileMatch(const std::string& inPath, const std::string& inPattern, const std::function<void(const fs::path&)>& onFound);
void forEachFileMatch(const std::string& inPath, const StringList& inPatterns, const std::function<void(const fs::path&)>& onFound);
void forEachFileMatch(const std::string& inPattern, const std::function<void(const fs::path&)>& onFound);
void forEachFileMatch(const StringList& inPatterns, const std::function<void(const fs::path&)>& onFound);

bool readFileAndReplace(const fs::path& inFile, const std::function<void(std::string&)>& onReplace);
std::string readShebangFromFile(const fs::path& inFile);
void sleep(const double inSeconds);

bool pathExists(const fs::path& inPath);
bool pathIsEmpty(const fs::path& inPath, const std::vector<fs::path>& inExceptions = {}, const bool inCheckExists = false);

bool createFileWithContents(const std::string& inFile, const std::string& inContents);

inline bool subprocess(const StringList& inCmd, const bool inCleanOutput = true);
inline bool subprocess(const StringList& inCmd, CreateSubprocessFunc inOnCreate, const bool inCleanOutput = true);
inline bool subprocess(const StringList& inCmd, std::string inCwd, const bool inCleanOutput = true);
inline bool subprocess(const StringList& inCmd, std::string inCwd, const PipeOption inStdErr, const bool inCleanOutput = true);
inline bool subprocess(const StringList& inCmd, std::string inCwd, const PipeOption inStdOut, const PipeOption inStdErr, const bool inCleanOutput = true);
inline bool subprocess(const StringList& inCmd, const PipeOption inStdOut, const bool inCleanOutput = true);
inline bool subprocess(const StringList& inCmd, const PipeOption inStdOut, const PipeOption inStdErr, const bool inCleanOutput = true);
inline bool subprocessNoOutput(const StringList& inCmd, const bool inCleanOutput = true);
inline bool subprocessNoOutput(const StringList& inCmd, std::string inCwd, const bool inCleanOutput = true);
bool subprocess(const StringList& inCmd, std::string inCwd, CreateSubprocessFunc inOnCreate, const PipeOption inStdOut, const PipeOption inStdErr, EnvMap inEnvMap, const bool inCleanOutput = true);
std::string subprocessOutput(const StringList& inCmd, const bool inCleanOutput = true, const PipeOption inStdErr = PipeOption::Pipe);
inline bool subprocessOutputToFile(const StringList& inCmd, const std::string& inOutputFile, const bool inCleanOutput = true);
bool subprocessOutputToFile(const StringList& inCmd, const std::string& inOutputFile, const PipeOption inStdErr, const bool inCleanOutput = true);

std::string which(const std::string& inExecutable, const bool inCleanOutput = true);

std::string testCompilerFlags(const std::string& inCompilerExec, const bool inCleanOutput = true);
#if defined(CHALET_WIN32)
const std::string& getCygPath();
#endif
#if defined(CHALET_MACOS)
const std::string& getXcodePath();
#endif
}
}

#include "Terminal/Commands.inl"

#endif // CHALET_COMMANDS_HPP
