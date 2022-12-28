/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Hash.hpp"

namespace chalet
{
namespace priv
{
/*****************************************************************************/
template <typename T>
void addArg(std::string& outString, T&& inArg)
{
	using Type = std::decay_t<T>;
	if constexpr (std::is_same_v<Type, std::string>)
	{
		if (!inArg.empty())
		{
			outString += inArg + '_';
		}
	}
	else if constexpr (std::is_enum_v<Type>)
	{
		outString += std::to_string(std::underlying_type_t<Type>(inArg)) + '_';
	}
	else
	{
		outString += std::to_string(inArg) + '_';
	}
}
}
/*****************************************************************************/
template <typename... Args>
std::string Hash::getHashableString(Args&&... args)
{
	std::string ret;

	((priv::addArg<Args>(ret, std::forward<Args>(args))), ...);

	ret.pop_back();

	return ret;
}
}
