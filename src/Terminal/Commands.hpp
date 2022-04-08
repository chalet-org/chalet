/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMANDS_HPP
#define CHALET_COMMANDS_HPP

#include "Process/PipeOption.hpp"
#include "Utility/GlobMatch.hpp"

namespace chalet
{
namespace Commands
{
using CreateSubprocessFunc = std::function<void(int /* pid */)>;

std::string getWorkingDirectory();
bool changeWorkingDirectory(const std::string& inPath);

bool pathIsFile(const std::string& inPath);
bool pathIsDirectory(const std::string& inPath);
bool pathIsSymLink(const std::string& inPath);

std::string getCanonicalPath(const std::string& inPath);
std::string getAbsolutePath(const std::string& inPath);
std::string getProximatePath(const std::string& inPath, const std::string& inBase);
std::string resolveSymlink(const std::string& inPath);

std::uintmax_t getPathSize(const std::string& inPath);
std::int64_t getLastWriteTime(const std::string& inFile);

bool makeDirectory(const std::string& inPath);
bool makeDirectories(const StringList& inPaths, bool& outDirectoriesMade);

bool remove(const std::string& inPath);
bool removeRecursively(const std::string& inPath);

bool setExecutableFlag(const std::string& inPath);
bool createDirectorySymbolicLink(const std::string& inFrom, const std::string& inTo);
bool createSymbolicLink(const std::string& inFrom, const std::string& inTo);

bool copy(const std::string& inFrom, const std::string& inTo, const fs::copy_options inOptions = fs::copy_options::overwrite_existing);
bool copySilent(const std::string& inFrom, const std::string& inTo, const fs::copy_options inOptions = fs::copy_options::overwrite_existing);
bool copyRename(const std::string& inFrom, const std::string& inTo, const bool inSilent = false);
bool rename(const std::string& inFrom, const std::string& inTo, const bool inSkipNonExisting = false);

bool forEachGlobMatch(const std::string& inPattern, const GlobMatch inSettings, const std::function<void(std::string)>& onFound);
bool forEachGlobMatch(const StringList& inPatterns, const GlobMatch inSettings, const std::function<void(std::string)>& onFound);
bool forEachGlobMatch(const std::string& inPath, const std::string& inPattern, const GlobMatch inSettings, const std::function<void(std::string)>& onFound);
bool forEachGlobMatch(const std::string& inPath, const StringList& inPatterns, const GlobMatch inSettings, const std::function<void(std::string)>& onFound);

bool addPathToListWithGlob(std::string&& inValue, StringList& outList, const GlobMatch inSettings);

bool readFileAndReplace(const std::string& inFile, const std::function<void(std::string&)>& onReplace);
std::string readShebangFromFile(const std::string& inFile);
void sleep(const double inSeconds);

bool pathExists(const std::string& inFile);
bool pathIsEmpty(const std::string& inPath, const std::vector<fs::path>& inExceptions = {});

bool createFileWithContents(const std::string& inFile, const std::string& inContents);

inline bool subprocess(const StringList& inCmd);
inline bool subprocess(const StringList& inCmd, CreateSubprocessFunc inOnCreate);
inline bool subprocess(const StringList& inCmd, std::string inCwd);
inline bool subprocess(const StringList& inCmd, std::string inCwd, const PipeOption inStdErr);
inline bool subprocess(const StringList& inCmd, std::string inCwd, const PipeOption inStdOut, const PipeOption inStdErr);
inline bool subprocess(const StringList& inCmd, const PipeOption inStdOut);
inline bool subprocess(const StringList& inCmd, const PipeOption inStdOut, const PipeOption inStdErr);
inline bool subprocessWithInput(const StringList& inCmd);
inline bool subprocessWithInput(const StringList& inCmd, CreateSubprocessFunc inOnCreate);
inline bool subprocessOutputToFile(const StringList& inCmd, const std::string& inOutputFile);

bool subprocess(const StringList& inCmd, std::string inCwd, CreateSubprocessFunc inOnCreate, const PipeOption inStdOut, const PipeOption inStdErr);
bool subprocessWithInput(const StringList& inCmd, std::string inCwd, CreateSubprocessFunc inOnCreate, const PipeOption inStdOut, const PipeOption inStdErr);
bool subprocessNoOutput(const StringList& inCmd);
bool subprocessMinimalOutput(const StringList& inCmd);
bool subprocessMinimalOutput(const StringList& inCmd, std::string inCwd);
bool subprocessOutputToFile(const StringList& inCmd, const std::string& inOutputFile, const PipeOption inStdErr);
std::string subprocessOutput(const StringList& inCmd, const PipeOption inStdOut = PipeOption::Pipe, const PipeOption inStdErr = PipeOption::Pipe);
std::string subprocessOutput(const StringList& inCmd, std::string inWorkingDirectory, const PipeOption inStdOut = PipeOption::Pipe, const PipeOption inStdErr = PipeOption::Pipe);

bool subprocessNinjaBuild(const StringList& inCmd, std::string inCwd = std::string());

std::string isolateVersion(const std::string& outString);
std::string which(const std::string& inExecutable);

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
