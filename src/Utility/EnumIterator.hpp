/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ENUM_ITERATOR_HPP
#define CHALET_ENUM_ITERATOR_HPP

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

#endif // CHALET_ENUM_ITERATOR_HPP
