/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/String.hpp"

#include <string.h>

namespace chalet
{
/*****************************************************************************/
inline bool String::equals(const std::string& inString, const char* inCompare)
{
	return ::strcmp(inString.c_str(), inCompare) == 0;
}
}
