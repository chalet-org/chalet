/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/EnumIterator.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T, T Beg, T End>
EnumIterator<T, Beg, End>::EnumIterator() :
	val(static_cast<std::underlying_type_t<T>>(Beg))
{
}

/*****************************************************************************/
template <typename T, T Beg, T End>
EnumIterator<T, Beg, End>::EnumIterator(T inValue) :
	val(static_cast<std::underlying_type_t<T>>(inValue))
{
}

/*****************************************************************************/
template <typename T, T Beg, T End>
EnumIterator<T, Beg, End> EnumIterator<T, Beg, End>::operator++()
{
	++val;
	return *this;
}

/*****************************************************************************/
template <typename T, T Beg, T End>
T EnumIterator<T, Beg, End>::operator*()
{
	return static_cast<T>(val);
}

/*****************************************************************************/
template <typename T, T Beg, T End>
EnumIterator<T, Beg, End> EnumIterator<T, Beg, End>::begin()
{
	return *this;
}

/*****************************************************************************/
template <typename T, T Beg, T End>
EnumIterator<T, Beg, End> EnumIterator<T, Beg, End>::end()
{
	return ++EnumIterator(End);
}

/*****************************************************************************/
template <typename T, T Beg, T End>
bool EnumIterator<T, Beg, End>::operator!=(const EnumIterator& rhs)
{
	return val != rhs.val;
}
}
