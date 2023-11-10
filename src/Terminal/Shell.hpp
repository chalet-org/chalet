/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
namespace Shell
{
bool isSubprocess();
bool isBash();
bool isBashGenericColorTermOrWindowsTerminal();
bool isMicrosoftTerminalOrWindowsBash();
bool isWindowsSubsystemForLinux();
bool isCommandPromptOrPowerShell();
bool isContinuousIntegrationServer();
bool isVisualStudioOutput();
bool isJetBrainsOutput();

std::string getNull();
}
}
