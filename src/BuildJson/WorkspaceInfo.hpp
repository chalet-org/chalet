/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WORKSPACE_INFO_HPP
#define CHALET_WORKSPACE_INFO_HPP

namespace chalet
{
struct WorkspaceInfo
{
	const std::string& workspace() const noexcept;
	void setWorkspace(const std::string& inValue) noexcept;

	const std::string& version() const noexcept;
	void setVersion(const std::string& inValue) noexcept;

	std::size_t hash() const noexcept;
	void setHash(const std::size_t inValue) noexcept;

private:
	std::string m_workspace;
	std::string m_version;
	std::size_t m_hash;
};
}

#endif // CHALET_WORKSPACE_INFO_HPP
