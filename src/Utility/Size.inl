/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Size.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
Size<T>::Size(const T inWidth, const T inHeight) :
	width(inWidth),
	height(inHeight)
{
}

}
