/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Path.hpp"

namespace chalet
{
/*****************************************************************************/
constexpr char Path::getSeparator()
{
#if defined(CHALET_WIN32)
	return ';';
#else
	return ':';
#endif
}
}