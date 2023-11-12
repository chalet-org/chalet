/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Utility/Version.hpp"

namespace chalet
{
class BuildState;
struct IBuildTarget;

struct TargetMetadata
{
	bool initialize(const BuildState& inState, const IBuildTarget* inTarget, const bool isWorkspace = false);

	const std::string& name() const noexcept;
	void setName(std::string&& inValue) noexcept;

	const std::string& versionString() const noexcept;
	const Version& version() const noexcept;
	void setVersion(std::string&& inValue) noexcept;

	const std::string& description() const noexcept;
	void setDescription(std::string&& inValue) noexcept;

	const std::string& homepage() const noexcept;
	void setHomepage(std::string&& inValue) noexcept;

	const std::string& author() const noexcept;
	void setAuthor(std::string&& inValue) noexcept;

	const std::string& license() const noexcept;
	void setLicense(std::string&& inValue) noexcept;

	const std::string& readme() const noexcept;
	void setReadme(std::string&& inValue) noexcept;

	std::string getHash() const;
	std::string getMetadataFromString(const std::string& inString) const;

private:
	std::string m_name;
	std::string m_versionString;
	Version m_version;

	std::string m_description;
	std::string m_homepage;
	std::string m_author;
	std::string m_license;
	std::string m_readme;
};
}
