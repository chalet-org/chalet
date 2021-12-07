/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Position.hpp"

namespace chalet
{
template <typename T>
Position<T>::Position(const T inX, const T inY) :
	x(inX),
	y(inY)
{
}
}
