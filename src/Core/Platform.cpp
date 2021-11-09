/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/Platform.hpp"

namespace chalet
{
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
}
