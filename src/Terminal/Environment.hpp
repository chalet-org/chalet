/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ENVIRONMENT_HPP
#define CHALET_ENVIRONMENT_HPP

namespace chalet
{
namespace Environment
{
bool isBash();
bool isBashGenericColorTermOrWindowsTerminal();
bool isMicrosoftTerminalOrWindowsBash();
bool isCommandPromptOrPowerShell();
bool isContinuousIntegrationServer();

const char* get(const char* inName);
std::string getAsString(const char* inName, std::string inFallback = std::string());
void set(const char* inName, const std::string& inValue);

void replaceCommonVariables(std::string& outString, const std::string& inHomeDirectory);

std::string getPath();
const char* getPathKey();
void setPath(const std::string& inValue);

std::string getUserDirectory();
std::string getShell();
std::string getComSpec();

bool saveToEnvFile(const std::string& inOutputFile);
void createDeltaEnvFile(const std::string& inBeforeFile, const std::string& inAfterFile, const std::string& inDeltaFile, const std::function<void(std::string&)>& onReadLine);
void readEnvFileToDictionary(const std::string& inDeltaFile, Dictionary<std::string>& outVariables);
}
}

#endif // CHALET_ENVIRONMENT_HPP
