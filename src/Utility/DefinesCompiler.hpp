/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DEFINES_COMPILER_HPP
#define CHALET_DEFINES_COMPILER_HPP

#if defined(_MSC_VER)
	#define CHALET_MSVC
#elif defined(__clang__)
	#define CHALET_CLANG
#endif

#endif // CHALET_DEFINES_COMPILER_HPP