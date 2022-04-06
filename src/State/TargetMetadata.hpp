/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_TARGET_METADATA_HPP
#define CHALET_TARGET_METADATA_HPP

#include "Utility/Version.hpp"

namespace chalet
{
struct TargetMetadata
{
	bool initialize();

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

#endif // CHALET_TARGET_METADATA_HPP
