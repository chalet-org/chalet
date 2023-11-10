/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include <type_traits>

namespace chalet
{
template <typename T, T Beg, T End>
class EnumIterator
{
	std::underlying_type_t<T> val;

public:
	EnumIterator();
	EnumIterator(T inValue);

	EnumIterator operator++();

	T operator*();

	EnumIterator begin();
	EnumIterator end();

	bool operator!=(const EnumIterator& rhs);
};

}

#include "Utility/EnumIterator.inl"
