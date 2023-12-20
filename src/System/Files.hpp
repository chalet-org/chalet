/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Utility/GlobMatch.hpp"

namespace chalet
{
namespace Files
{
using GlobCallback = std::function<void(const std::string&)>;

std::string getPlatformExecutableExtension();
std::string getPlatformSharedLibraryExtension();
std::string getPlatformFrameworkExtension();

std::string getWorkingDirectory();
bool changeWorkingDirectory(const std::string& inPath);

bool pathIsFile(const std::string& inPath);
bool pathIsDirectory(const std::string& inPath);
bool pathIsSymLink(const std::string& inPath);

std::string getCanonicalPath(const std::string& inPath);
std::string getAbsolutePath(const std::string& inPath);
std::string getProximatePath(const std::string& inPath, const std::string& inBase);
std::string resolveSymlink(const std::string& inPath);

uintmax_t getPathSize(const std::string& inPath);
i64 getLastWriteTime(const std::string& inFile);

bool makeDirectory(const std::string& inPath);
bool makeDirectories(const StringList& inPaths);

bool removeIfExists(const std::string& inPath);
bool removeRecursively(const std::string& inPath);

bool setExecutableFlag(const std::string& inPath);
bool createDirectorySymbolicLink(const std::string& inFrom, const std::string& inTo);
bool createSymbolicLink(const std::string& inFrom, const std::string& inTo);

bool copy(const std::string& inFrom, const std::string& inTo, const fs::copy_options inOptions = fs::copy_options::overwrite_existing);
bool copySilent(const std::string& inFrom, const std::string& inTo, const fs::copy_options inOptions = fs::copy_options::overwrite_existing);
bool copyRename(const std::string& inFrom, const std::string& inTo, const bool inSilent = false);
bool moveSilent(const std::string& inFrom, const std::string& inTo, const fs::copy_options inOptions = fs::copy_options::overwrite_existing);
bool rename(const std::string& inFrom, const std::string& inTo, const bool inSkipNonExisting = false);

bool forEachGlobMatch(const std::string& inPattern, const GlobMatch inSettings, const GlobCallback& onFound);
bool forEachGlobMatch(const StringList& inPatterns, const GlobMatch inSettings, const GlobCallback& onFound);
bool forEachGlobMatch(const std::string& inPath, const std::string& inPattern, const GlobMatch inSettings, const GlobCallback& onFound);
bool forEachGlobMatch(const std::string& inPath, const StringList& inPatterns, const GlobMatch inSettings, const GlobCallback& onFound);

bool addPathToListWithGlob(const std::string& inValue, StringList& outList, const GlobMatch inSettings);

bool readFileAndReplace(const std::string& inFile, const std::function<void(std::string&)>& onReplace);
void sleep(const f64 inSeconds);

bool pathExists(const std::string& inFile);
bool pathIsEmpty(const std::string& inPath, const std::vector<fs::path>& inExceptions = {});

bool createFileWithContents(const std::string& inFile, const std::string& inContents);
std::string getFileContents(const std::string& inFile);

std::string getFirstChildDirectory(const std::string& inPath);

std::string isolateVersion(const std::string& outString);
std::string which(const std::string& inExecutable, const bool inOutput = true);

#if defined(CHALET_WIN32)
const std::string& getCygPath();
#endif
#if defined(CHALET_MACOS)
const std::string& getXcodePath();
bool isUsingAppleCommandLineTools();
#endif
}
}
