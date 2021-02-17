/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SUBPROCESS_API_HPP
#define CHALET_SUBPROCESS_API_HPP

#ifdef CHALET_MSVC
	#pragma warning(push)
	#pragma warning(disable : 4456)
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wshadow"
#endif

#include <subprocess/subprocess.hpp>

#ifdef CHALET_MSVC
	#pragma warning(pop)
#else
	#pragma GCC diagnostic pop
#endif

namespace chalet
{
namespace sp = subprocess;
}

#endif // CHALET_SUBPROCESS_API_HPP
