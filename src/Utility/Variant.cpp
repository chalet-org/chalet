/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Variant.hpp"

#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
Variant::Variant(StringList&& inValue)
{
	m_value = std::move(inValue);

	if (m_kind != Kind::StringList)
		m_kind = Kind::StringList;
}

/*****************************************************************************/
Variant::Variant(const Kind inKind) :
	m_kind(inKind)
{
	switch (m_kind)
	{
		case Variant::Kind::StringList:
			m_value = StringList();
			break;

		case Variant::Kind::String:
			m_value = std::string();
			break;

		case Variant::Kind::OptionalBoolean:
			m_value = std::optional<bool>();
			break;

		case Variant::Kind::Boolean:
			m_value = false;
			break;

		case Variant::Kind::OptionalInteger:
			m_value = std::optional<i32>();
			break;

		case Variant::Kind::Integer:
			m_value = 0;
			break;

		case Variant::Kind::Empty:
		case Variant::Kind::Enum:
		default:
			m_value = std::any();
			break;
	}
}

/*****************************************************************************/
Variant::Kind Variant::kind() const noexcept
{
	return m_kind;
}

/*****************************************************************************/
Variant::operator bool() const
{
	return m_value.has_value();
}

/*****************************************************************************/
bool Variant::asBool() const
{
	if (m_kind == Kind::Boolean)
	{
		return std::any_cast<bool>(m_value);
	}

	return false;
}

/*****************************************************************************/
std::optional<bool> Variant::asOptionalBool() const
{
	if (m_kind == Kind::OptionalBoolean)
	{
		return std::any_cast<std::optional<bool>>(m_value);
	}

	return std::nullopt;
}

/*****************************************************************************/
i32 Variant::asInt() const
{
	if (m_kind == Kind::Integer)
	{
		return std::any_cast<i32>(m_value);
	}

	return 0;
}

/*****************************************************************************/
std::optional<i32> Variant::asOptionalInt() const
{
	if (m_kind == Kind::OptionalInteger)
	{
		return std::any_cast<std::optional<i32>>(m_value);
	}

	return std::nullopt;
}

/*****************************************************************************/
std::string Variant::asString() const
{
	if (m_kind == Kind::String)
	{
		return std::any_cast<std::string>(m_value);
	}

	return std::string();
}

/*****************************************************************************/
StringList Variant::asStringList() const
{
	if (m_kind == Kind::StringList)
	{
		return std::any_cast<StringList>(m_value);
	}

	return StringList();
}

/*****************************************************************************/
std::ostream& operator<<(std::ostream& os, const Variant& dt)
{
	switch (dt.m_kind)
	{
		case Variant::Kind::StringList:
			os << String::join(dt.asStringList());
			break;

		case Variant::Kind::String:
			os << dt.asString();
			break;

		case Variant::Kind::Boolean:
			os << (dt.asBool() ? "true" : "false");
			break;

		case Variant::Kind::OptionalBoolean: {
			auto raw = dt.asOptionalBool();
			if (!raw.has_value())
			{
				os << "nullopt";
			}
			else
			{
				bool val = *raw;
				os << (val ? "true" : "false");
			}
			break;
		}

		case Variant::Kind::Integer:
			os << dt.asInt();
			break;

		case Variant::Kind::OptionalInteger: {
			auto raw = dt.asOptionalInt();
			if (!raw.has_value())
			{
				os << "nullopt";
			}
			else
			{
				i32 val = *raw;
				os << val;
			}
			break;
		}

		case Variant::Kind::Enum:
			os << dt.asInt();
			break;

		case Variant::Kind::Empty:
		default:
			break;
	}
	return os;
}
}
