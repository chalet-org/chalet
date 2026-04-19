/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Platform/Platform.hpp"

#include "Core/CommandLineInputs.hpp"

namespace chalet
{
/*****************************************************************************/
StringList Platform::validPlatforms() noexcept
{
	return {
		"windows",
		"macos",
		"linux",
		"web"
	};
}

/*****************************************************************************/
std::string Platform::platform() noexcept
{
#if defined(CHALET_WIN32)
	return "windows";
#elif defined(CHALET_MACOS)
	return "macos";
#elif defined(CHALET_LINUX)
	return "linux";
#else
	#error "Unknown platform"
	return "unknown";
#endif
}

/*****************************************************************************/
void Platform::assignPlatform(const CommandLineInputs& inInputs, std::string& outPlatform)
{
	bool isWeb = inInputs.toolchainPreference().type == ToolchainType::Emscripten;
	if (isWeb)
	{
		outPlatform = "web";
	}
	else
	{
		outPlatform = Platform::platform();
	}
}

/*****************************************************************************/
StringList Platform::getDefaultPlatformDefines()
{
	StringList ret;

#if defined(CHALET_WIN32)
	ret.emplace_back("_WIN32");
#elif defined(CHALET_MACOS)
	ret.emplace_back("__APPLE__");
#elif defined(CHALET_LINUX)
	ret.emplace_back("__linux__");
#else
	#error "Unknown platform"
#endif

	return ret;
}

/*****************************************************************************/
bool Platform::isLittleEndian() noexcept
{
	i32 n = 1;
	return *(char*)&n == 1;
}
bool Platform::isBigEndian() noexcept
{
	return !isLittleEndian();
}
}
