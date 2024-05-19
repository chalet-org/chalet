/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PRECOMPILED_HEADER_HPP
#define CHALET_PRECOMPILED_HEADER_HPP

#ifdef __cplusplus

	#ifndef _DEBUG
		#ifndef NDEBUG
			#define NDEBUG
		#endif
	#endif // _DEBUG

	// Typical stdafx.h
	#include <algorithm>
	#include <cstdio>
	#include <deque>
	#include <fstream>
	#include <functional>
	#include <iostream>
	#include <list>
	#include <map>
	#include <memory>
	#include <set>
	#include <string>
	#include <unordered_map>
	#include <unordered_set>
	#include <vector>

	// // Additional C/C++ libs
	#include <cassert>
	#include <sstream>
	#include <optional>

	// Defines
	#include "System/DefinesCompiler.hpp"
	#include "System/DefinesPlatform.hpp"
	#include "System/DefinesEnabledFeatures.hpp"
	//
	#include "System/DefinesExceptions.hpp"

	// Utils
	#include "System/Types.hpp"

	#include "Libraries/FileSystem.hpp"
	#include "Libraries/Format.hpp"

	#include "System/Diagnostic.hpp"
	#include "System/Logger.hpp"
	#include "System/Macros.hpp"
	#include "System/Unused.hpp"

#endif // __cplusplus

#endif // CHALET_PRECOMPILED_HEADER_HPP
