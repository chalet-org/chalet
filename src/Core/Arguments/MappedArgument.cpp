/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/Arguments/MappedArgument.hpp"

#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
MappedArgument::MappedArgument(ArgumentIdentifier inId, Variant inValue) :
	m_id(inId),
	m_value(std::move(inValue))
{
}

/*****************************************************************************/
ArgumentIdentifier MappedArgument::id() const noexcept
{
	return m_id;
}

/*****************************************************************************/
const std::string& MappedArgument::key() const noexcept
{
	return m_key;
}

/*****************************************************************************/
const std::string& MappedArgument::keyLong() const noexcept
{
	return m_keyLong;
}

const std::string& MappedArgument::keyLabel() const noexcept
{
	return m_keyLabel;
}

const Variant& MappedArgument::value() const noexcept
{
	return m_value;
}

/*****************************************************************************/
MappedArgument& MappedArgument::addBooleanArgument(std::string inOption)
{
	if (!String::startsWith("--[no-]", inOption))
		return addArgument(inOption);

	auto baseArgument = inOption.substr(7);
	chalet_assert(baseArgument.size() > 0, "");

	m_key = fmt::format("--{}", baseArgument);
	m_keyLong = fmt::format("--no-{}", baseArgument);
	m_keyLabel = inOption;

	return *this;
}

/*****************************************************************************/
MappedArgument& MappedArgument::addArgument(std::string_view inOption)
{
	m_key = inOption;
	m_keyLong.clear();

	return *this;
}

MappedArgument& MappedArgument::addArgument(std::string_view inShort, std::string_view inLong)
{
	m_key = inShort;
	m_keyLong = inLong;

	return *this;
}

/*****************************************************************************/
const std::string& MappedArgument::help() const noexcept
{
	return m_help;
}

MappedArgument& MappedArgument::setHelp(std::string&& inHelp)
{
	m_help = std::move(inHelp);

	return *this;
}

/*****************************************************************************/
bool MappedArgument::required() const noexcept
{
	return m_required;
}

MappedArgument& MappedArgument::setRequired(const bool inValue)
{
	m_required = inValue;

	return *this;
}
}
