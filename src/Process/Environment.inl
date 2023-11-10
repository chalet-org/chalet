/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Process/Environment.hpp"

namespace chalet
{
/*****************************************************************************/
constexpr char Environment::getPathSeparator()
{
#if defined(CHALET_WIN32)
	return ';';
#else
	return ':';
#endif
}
}