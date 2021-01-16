/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/WorkspaceInfo.hpp"

namespace chalet
{
/*****************************************************************************/
const std::string& WorkspaceInfo::workspace() const noexcept
{
	return m_workspace;
}

void WorkspaceInfo::setWorkspace(const std::string& inValue) noexcept
{
	m_workspace = inValue;
}

/*****************************************************************************/
const std::string& WorkspaceInfo::version() const noexcept
{
	return m_version;
}
void WorkspaceInfo::setVersion(const std::string& inValue) noexcept
{
	m_version = inValue;
}

/*****************************************************************************/
std::size_t WorkspaceInfo::hash() const noexcept
{
	return m_hash;
}
void WorkspaceInfo::setHash(const std::size_t inValue) noexcept
{
	m_hash = inValue;
}
}