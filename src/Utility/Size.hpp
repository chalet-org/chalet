/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
template <typename T>
struct Size
{
	Size() = default;
	explicit Size(const T inWidth, const T inHeight);

	T width = static_cast<T>(0);
	T height = static_cast<T>(0);
};
}

#include "Utility/Size.inl"

namespace chalet
{
extern template struct Size<u16>;
}
