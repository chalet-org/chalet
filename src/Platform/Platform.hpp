/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct CommandLineInputs;

namespace Platform
{
StringList validPlatforms() noexcept;
std::string platform() noexcept;
StringList notPlatforms() noexcept;
void assignPlatform(const CommandLineInputs& inInputs, std::string& outPlatform, StringList& outNotPlatforms);
StringList getDefaultPlatformDefines();
bool isLittleEndian() noexcept;
bool isBigEndian() noexcept;
}
}
