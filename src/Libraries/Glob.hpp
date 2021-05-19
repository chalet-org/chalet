/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_GLOB_HPP
#define CHALET_GLOB_HPP

#include "Libraries/FileSystem.hpp"

#ifdef CHALET_MSVC
	#pragma warning(push)
	#pragma warning(disable : 4100)
	#pragma warning(disable : 4101)
	#pragma warning(disable : 4456)
	#pragma warning(disable : 4996)
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-parameter"
	#pragma GCC diagnostic ignored "-Wshadow"
#endif

#include <glob/glob.hpp>

#ifdef CHALET_MSVC
	#pragma warning(pop)
#else
	#pragma GCC diagnostic pop
#endif

#endif // CHALET_GLOB_HPP
