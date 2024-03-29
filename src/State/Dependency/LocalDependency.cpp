/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Dependency/LocalDependency.hpp"

#include "Core/CommandLineInputs.hpp"

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
const std::string& LocalDependency::getHash() const
{
	if (m_hash.empty())
	{
		auto hashable = Hash::getHashableString(this->name(), m_path);
		m_hash = Hash::string(hashable);
	}

	return m_hash;
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
