/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Dependency/IBuildDependency.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "State/StatePrototype.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
IBuildDependency::IBuildDependency(const StatePrototype& inPrototype, const BuildDependencyType inType) :
	m_prototype(inPrototype),
	m_type(inType)
{
}

/*****************************************************************************/
[[nodiscard]] BuildDependency IBuildDependency::make(const BuildDependencyType inType, const CommandLineInputs& inInputs, const StatePrototype& inPrototype)
{
	switch (inType)
	{
		case BuildDependencyType::Git:
			return std::make_unique<GitDependency>(inInputs, inPrototype);
		case BuildDependencyType::SVN:
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented BuildTargetType requested: ", static_cast<int>(inType));
	return nullptr;
}

/*****************************************************************************/
BuildDependencyType IBuildDependency::type() const noexcept
{
	return m_type;
}

/*****************************************************************************/
bool IBuildDependency::isGit() const noexcept
{
	return m_type == BuildDependencyType::Git;
}

/*****************************************************************************/
const std::string& IBuildDependency::name() const noexcept
{
	return m_name;
}
void IBuildDependency::setName(const std::string& inValue) noexcept
{
	if (String::startsWith('.', inValue) || String::startsWith('_', inValue) || String::startsWith('-', inValue) || String::startsWith('+', inValue))
		return;

	m_name = inValue;
}

}
