/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Dependency/LocalDependency.hpp"

#include "Core/CommandLineInputs.hpp"

#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "System/Files.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
LocalDependency::LocalDependency(const CentralState& inCentralState) :
	IExternalDependency(inCentralState, ExternalDependencyType::Local)
{
}

/*****************************************************************************/
bool LocalDependency::initialize()
{
	if (!m_centralState.replaceVariablesInString(m_path, this))
		return false;

	return true;
}

/*****************************************************************************/
bool LocalDependency::validate()
{
	if (m_path.empty())
	{
		Diagnostic::error("The local dependency path was blank for '{}'.", this->name());
		return false;
	}

	if (!Files::pathExists(m_path))
	{
		Diagnostic::error("The local dependency path for '{}' does not exist.", this->name());
		return false;
	}

	return true;
}

/*****************************************************************************/
std::string LocalDependency::getStateHash(const BuildState& inState) const
{
	auto configHash = inState.configuration.getHash();
	auto hashable = Hash::getHashableString(this->name(), m_path, configHash);
	return Hash::string(hashable);
}

/*****************************************************************************/
const std::string& LocalDependency::path() const noexcept
{
	return m_path;
}

void LocalDependency::setPath(std::string&& inValue) noexcept
{
	m_path = std::move(inValue);
}

}
