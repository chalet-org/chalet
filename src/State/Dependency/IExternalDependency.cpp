/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Dependency/IExternalDependency.hpp"

#include "State/CentralState.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
IExternalDependency::IExternalDependency(const CentralState& inCentralState, const ExternalDependencyType inType) :
	m_centralState(inCentralState),
	m_type(inType)
{
}

/*****************************************************************************/
[[nodiscard]] ExternalDependency IExternalDependency::make(const ExternalDependencyType inType, const CentralState& inCentralState)
{
	switch (inType)
	{
		case ExternalDependencyType::Git:
			return std::make_unique<GitDependency>(inCentralState);
		case ExternalDependencyType::SVN:
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented BuildTargetType requested: {}", static_cast<int>(inType));
	return nullptr;
}

/*****************************************************************************/
ExternalDependencyType IExternalDependency::type() const noexcept
{
	return m_type;
}

/*****************************************************************************/
bool IExternalDependency::isGit() const noexcept
{
	return m_type == ExternalDependencyType::Git;
}

/*****************************************************************************/
const std::string& IExternalDependency::name() const noexcept
{
	return m_name;
}
void IExternalDependency::setName(const std::string& inValue) noexcept
{
	if (String::startsWith('.', inValue) || String::startsWith('_', inValue) || String::startsWith('-', inValue) || String::startsWith('+', inValue))
		return;

	m_name = inValue;
}

}
