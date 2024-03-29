/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Core/Arguments/ArgumentIdentifier.hpp"
#include "Utility/Variant.hpp"

namespace chalet
{
struct MappedArgument
{
	MappedArgument(ArgumentIdentifier inId, Variant inValue);

	ArgumentIdentifier id() const noexcept;

	const std::string& key() const noexcept;
	const std::string& keyLong() const noexcept;
	const std::string& keyLabel() const noexcept;
	const Variant& value() const noexcept;

	MappedArgument& addBooleanArgument(std::string inOption);
	MappedArgument& addArgument(std::string_view inOption);
	MappedArgument& addArgument(std::string_view inShort, std::string_view inLong);

	const std::string& help() const noexcept;
	MappedArgument& setHelp(std::string&& inHelp);

	bool required() const noexcept;
	MappedArgument& setRequired(const bool inValue = true);

	template <typename T>
	MappedArgument& setValue(T&& inValue);

private:
	ArgumentIdentifier m_id = ArgumentIdentifier::None;
	Variant m_value;

	std::string m_key;
	std::string m_keyLong;
	std::string m_keyLabel;

	std::string m_help;

	bool m_required = false;
};
}

#include "Core/Arguments/MappedArgument.inl"
