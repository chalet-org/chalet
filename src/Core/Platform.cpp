/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/Platform.hpp"

namespace chalet
{
namespace
{
static struct
{
	std::string platform;
	StringList notPlatforms;

} state;

std::string getPlatform()
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

StringList getNotPlatforms()
{
#if defined(CHALET_WIN32)
	return {
		"macos", "linux"
	};
#elif defined(CHALET_MACOS)
	return {
		"windows", "linux"
	};
#elif defined(CHALET_LINUX)
	return {
		"windows", "macos"
	};
#else
	return {
		"windows", "macos", "linux"
	};
#endif
}
}

/*****************************************************************************/
const std::string& Platform::platform() noexcept
{
	if (state.platform.empty())
	{
		state.platform = getPlatform();
	}

	return state.platform;
}

/*****************************************************************************/
const StringList& Platform::notPlatforms() noexcept
{
	if (state.notPlatforms.empty())
	{
		state.notPlatforms = getNotPlatforms();
	}

	return state.notPlatforms;
}
}
