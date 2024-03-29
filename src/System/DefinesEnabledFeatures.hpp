/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "System/DefinesPlatform.hpp"

#if defined(CHALET_WIN32)
	#define CHALET_ENABLE_INTEL_ICC 0
	#define CHALET_ENABLE_INTEL_ICX 1
#elif defined(CHALET_MACOS)
	#define CHALET_ENABLE_INTEL_ICC 1
	#define CHALET_ENABLE_INTEL_ICX 0
#else
	#define CHALET_ENABLE_INTEL_ICC 0
	#define CHALET_ENABLE_INTEL_ICX 0
#endif
