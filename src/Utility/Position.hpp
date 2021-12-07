/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_POSITION_HPP
#define CHALET_POSITION_HPP

namespace chalet
{
template <typename T>
struct Position
{
	Position() = default;
	explicit Position(const T inX, const T inY);

	T x = static_cast<T>(0);
	T y = static_cast<T>(0);
};
}

#include "Utility/Position.inl"

namespace chalet
{
extern template struct Position<short>;
}

#endif // CHALET_POSITION_HPP
