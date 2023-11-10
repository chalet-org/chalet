/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/VariableAdapter.hpp"

#include "Process/Environment.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
void VariableAdapter::set(const std::string& inKey, std::string&& inValue)
{
	if (String::contains("${", inValue))
	{
		// Note: we don't care about this result, because if the signing identity is blank,
		//   the application simply won't be signed
		//
		UNUSED(RegexPatterns::matchAndReplacePathVariables(inValue, [this](std::string match, bool& required) {
			required = false;

			if (String::startsWith("env:", match))
			{
				match = match.substr(4);
				return Environment::getString(match.c_str());
			}

			if (String::startsWith("var:", match))
			{
				match = match.substr(4);
				return this->get(match);
			}

			return std::string();
		}));
	}

	m_variables[inKey] = std::move(inValue);
}

/*****************************************************************************/
const std::string& VariableAdapter::get(const std::string& inKey) const
{
	if (m_variables.find(inKey) != m_variables.end())
	{
		return m_variables.at(inKey);
	}

	return m_empty;
}

/*****************************************************************************/
bool VariableAdapter::contains(const std::string& inKey) const
{
	return m_variables.find(inKey) != m_variables.end();
}
}
