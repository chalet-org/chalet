/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Arguments/MappedArgument.hpp"

#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
MappedArgument::MappedArgument(Variant inValue) :
	m_value(std::move(inValue))
{
}

/*****************************************************************************/
std::string MappedArgument::key() const
{
	return String::join(m_options);
}

const Variant& MappedArgument::value() const noexcept
{
	return m_value;
}

/*****************************************************************************/
bool MappedArgument::is(const std::string& inOption) const
{
	return String::equals(m_options, inOption);
}

/*****************************************************************************/
MappedArgument& MappedArgument::addArgument(std::string_view inOption)
{
	m_options.clear();
	m_options.emplace_back(inOption);

	return *this;
}

MappedArgument& MappedArgument::addArgument(std::string_view inShort, std::string_view inLong)
{
	m_options.clear();
	m_options.emplace_back(inShort);
	m_options.emplace_back(inLong);

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
