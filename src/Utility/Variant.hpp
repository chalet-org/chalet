/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ARGUMENT_HPP
#define CHALET_ARGUMENT_HPP

#include <any>

#ifdef CHALET_MSVC
	#pragma warning(push)
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wshadow"
#endif

namespace chalet
{
struct Variant
{
	enum class Kind : uchar
	{
		Empty,
		Boolean,
		Integer,
		String,
		Enum,
		StringList,
		Remainder
	};

	Variant() = delete;
	Variant(StringList&& inValue);
	Variant(const Kind inKind);

	template <typename T>
	Variant(T&& inValue);

	Kind kind() const noexcept;

	bool asBool() const;
	int asInt() const;
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

#ifdef CHALET_MSVC
	#pragma warning(pop)
#else
	#pragma GCC diagnostic pop
#endif

#endif // CHALET_ARGUMENT_HPP
