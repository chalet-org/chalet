/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
namespace Environment
{
const char* get(const char* inName);
std::string getString(const char* inName);
std::string getString(const char* inName, const std::string& inFallback);
void set(const char* inName, const std::string& inValue);

void replaceCommonVariables(std::string& outString, const std::string& inHomeDirectory);

std::string getPath();
const char* getPathKey();
const char* getLibraryPathKey();
const char* getFrameworkPathKey();
void setPath(const std::string& inValue);
constexpr char getPathSeparator();

std::string getUserDirectory();
std::string getShell();
std::string getComSpec();

bool saveToEnvFile(const std::string& inOutputFile);
void createDeltaEnvFile(const std::string& inBeforeFile, const std::string& inAfterFile, const std::string& inDeltaFile, const std::function<void(std::string&)>& onReadLine);
void readEnvFileToDictionary(const std::string& inDeltaFile, Dictionary<std::string>& outVariables);
}
}

#include "Process/Environment.inl"
