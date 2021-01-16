/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PRECOMPILED_HEADER_HPP
#define CHALET_PRECOMPILED_HEADER_HPP

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
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

// // Additional C/C++ libs
#include <cassert>
#include <sstream>

// Defines
#include "Utility/Defines.hpp"

// Utils
#include "Utility/Types.hpp"

#include "Libraries/FileSystem.hpp"
#include "Terminal/Diagnostic.hpp"
#include "Utility/Logger.hpp"
#include "Utility/Unused.hpp"

namespace chalet
{
using StringList = std::vector<std::string>;
}

#endif // CHALET_PRECOMPILED_HEADER_HPP
