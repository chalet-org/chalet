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
std::string getWorkingDirectory();
fs::path getWorkingDirectoryPath();

std::string getCanonicalPath(const std::string& inPath);
std::string getAbsolutePath(const std::string& inPath);

bool makeDirectory(const std::string& inPath, const bool inCleanOutput = true);
bool makeDirectories(const StringList& inPaths, const bool inCleanOutput = true);
bool remove(const std::string& inPath, const bool inCleanOutput = true);
bool removeRecursively(const std::string& inPath, const bool inCleanOutput = true);
bool setExecutableFlag(const std::string& inPath, const bool inCleanOutput = true);
bool createDirectorySymbolicLink(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput = true);
bool createSymbolicLink(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput = true);
bool copy(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput = true);
bool copyRename(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput = true);
bool rename(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput = true);
void forEachFileMatch(const std::string& inPath, const std::string& inPattern, const std::function<void(const fs::path&)>& onFound);
void forEachFileMatch(const std::string& inPath, const StringList& inPatterns, const std::function<void(const fs::path&)>& onFound);
void forEachFileMatch(const std::string& inPattern, const std::function<void(const fs::path&)>& onFound);
void forEachFileMatch(const StringList& inPatterns, const std::function<void(const fs::path&)>& onFound);
bool readFileAndReplace(const fs::path& inFile, const std::function<void(std::string&)>& onReplace);

bool pathExists(const fs::path& inPath);
bool pathIsEmpty(const fs::path& inPath, const bool inCheckExists = false);

bool createFileWithContents(const std::string& inFile, const std::string& inContents);

bool subprocess(const StringList& inCmd, const bool inCleanOutput = true, const PipeOption inStdErr = PipeOption::StdErr, const PipeOption inStdOut = PipeOption::StdOut, EnvMap inEnvMap = EnvMap(), std::string inCwd = std::string());
std::string subprocessOutput(const StringList& inCmd, const bool inCleanOutput = true, const PipeOption inStdErr = PipeOption::Pipe, std::string inCwd = std::string());
bool shell(const std::string& inCmd, const bool inCleanOutput = true);
// bool shellAlternate(const std::string& inCmd, const bool inCleanOutput = true);
// std::string shellWithOutput(const std::string& inCmd, const bool inCleanOutput = true);
bool shellRemove(const std::string& inPath, const bool inCleanOutput = true);

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

#endif // CHALET_COMMANDS_HPP
