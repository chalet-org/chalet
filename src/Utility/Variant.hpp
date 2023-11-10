/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include <any>

#if defined(CHALET_MSVC)
	#pragma warning(push)
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wshadow"
#endif

namespace chalet
{
struct Variant
{
	enum class Kind : u8
	{
		Empty,
		Boolean,
		OptionalBoolean,
		Integer,
		OptionalInteger,
		String,
		Enum,
		StringList
	};

	Variant() = delete;
	Variant(StringList&& inValue);
	Variant(const Kind inKind);

	template <typename T>
	Variant(T&& inValue);

	Kind kind() const noexcept;
	operator bool() const;

	bool asBool() const;
	std::optional<bool> asOptionalBool() const;
	i32 asInt() const;
	std::optional<i32> asOptionalInt() const;
	std::string asString() const;
	StringList asStringList() const;

	template <typename T>
	T asEnum() const;

	friend std::ostream& operator<<(std::ostream& os, const Variant& dt);

private:
	std::any m_value;
	Kind m_kind = Kind::Empty;
};
}

#include "Utility/Variant.inl"

#if defined(CHALET_MSVC)
	#pragma warning(pop)
#else
	#pragma GCC diagnostic pop
#endif
