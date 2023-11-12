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
	return "unknown";
#endif
}

/*****************************************************************************/
StringList Platform::notPlatforms() noexcept
{
	return
	{
#if !defined(CHALET_WIN32)
		"windows",
#endif
#if !defined(CHALET_MACOS)
			"macos",
#endif
#if !defined(CHALET_LINUX)
			"linux",
#endif
	};
}

/*****************************************************************************/
void Platform::assignPlatform(const CommandLineInputs& inInputs, std::string& outPlatform, StringList& outNotPlatforms)
{
	outNotPlatforms = Platform::notPlatforms();

	bool isWeb = inInputs.toolchainPreference().type == ToolchainType::Emscripten;
	if (isWeb)
	{
		outPlatform = "web";
		outNotPlatforms.push_back(Platform::platform());
	}
	else
	{
		outPlatform = Platform::platform();
		outNotPlatforms.push_back("web");
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
#else
	ret.emplace_back("__linux__");
#endif

	return ret;
}
}
