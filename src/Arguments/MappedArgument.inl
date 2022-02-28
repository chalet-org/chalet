/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Arguments/MappedArgument.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
MappedArgument& MappedArgument::setValue(T&& inValue)
{
	m_value = std::forward<T>(inValue);

	return *this;
}
}
