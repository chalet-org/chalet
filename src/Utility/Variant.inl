/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Variant.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
Variant::Variant(T&& inValue)
{
	using Type = std::decay_t<T>;
	if constexpr (std::is_same_v<Type, std::string>)
	{
		m_value = std::move(inValue);
		m_kind = Kind::String;
	}
	else if constexpr (std::is_enum_v<Type>)
	{
		m_value = inValue;
		m_kind = Kind::Enum;
	}
	else if constexpr (std::is_same_v<Type, std::optional<bool>>)
	{
		m_value = inValue;
		m_kind = Kind::OptionalBoolean;
	}
	else if constexpr (std::is_same_v<Type, bool>)
	{
		m_value = inValue;
		m_kind = Kind::Boolean;
	}
	else if constexpr (std::is_same_v<Type, std::optional<int>>)
	{
		m_value = inValue;
		m_kind = Kind::OptionalInteger;
	}
	else if constexpr (std::is_same_v<Type, int>)
	{
		m_value = inValue;
		m_kind = Kind::Integer;
	}
	else if constexpr (std::is_same_v<Type, StringList>)
	{
		m_value = std::move(inValue);
		m_kind = Kind::StringList;
	}
	else
	{
		m_kind = Kind::Empty;
	}
}

/*****************************************************************************/
template <typename T>
T Variant::asEnum() const
{
	if (m_kind == Kind::Enum)
	{
		return std::any_cast<T>(m_value);
	}

	return static_cast<T>(0);
}
}
