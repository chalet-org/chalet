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
	return std::string("windows");
#elif defined(CHALET_MACOS)
	return std::string("macos");
#elif defined(CHALET_LINUX)
	return std::string("linux");
#else
	return std::string("unknown");
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
}
