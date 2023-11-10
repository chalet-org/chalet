/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct VariableAdapter
{
	VariableAdapter() = default;

	void set(const std::string& inKey, std::string&& inValue);

	const std::string& get(const std::string& inKey) const;

	bool contains(const std::string& inKey) const;

private:
	std::string m_empty;

	Dictionary<std::string> m_variables;
};
}
