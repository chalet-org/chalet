/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/TargetMetadata.hpp"

#include "Utility/Hash.hpp"

#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool TargetMetadata::initialize()
{
	if (!m_versionString.empty())
	{
		if (!m_version.setFromString(m_versionString))
		{
			Diagnostic::error("The supplied workspace version was invalid.");
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
const std::string& TargetMetadata::name() const noexcept
{
	return m_name;
}
void TargetMetadata::setName(std::string&& inValue) noexcept
{
	m_name = std::move(inValue);
}

/*****************************************************************************/
const std::string& TargetMetadata::versionString() const noexcept
{
	return m_versionString;
}
const Version& TargetMetadata::version() const noexcept
{
	return m_version;
}
void TargetMetadata::setVersion(std::string&& inValue) noexcept
{
	m_versionString = std::move(inValue);
}

/*****************************************************************************/
const std::string& TargetMetadata::description() const noexcept
{
	return m_description;
}
void TargetMetadata::setDescription(std::string&& inValue) noexcept
{
	m_description = std::move(inValue);
}

/*****************************************************************************/
const std::string& TargetMetadata::homepage() const noexcept
{
	return m_homepage;
}
void TargetMetadata::setHomepage(std::string&& inValue) noexcept
{
	m_homepage = std::move(inValue);
}

/*****************************************************************************/
const std::string& TargetMetadata::author() const noexcept
{
	return m_author;
}
void TargetMetadata::setAuthor(std::string&& inValue) noexcept
{
	m_author = std::move(inValue);
}

/*****************************************************************************/
const std::string& TargetMetadata::license() const noexcept
{
	return m_license;
}
void TargetMetadata::setLicense(std::string&& inValue) noexcept
{
	m_license = std::move(inValue);
}

/*****************************************************************************/
const std::string& TargetMetadata::readme() const noexcept
{
	return m_readme;
}
void TargetMetadata::setReadme(std::string&& inValue) noexcept
{
	m_readme = std::move(inValue);
}

/*****************************************************************************/
std::string TargetMetadata::getHash() const
{
	return Hash::string(fmt::format("{}{}{}{}{}{}{}", m_versionString, m_name, m_description, m_homepage, m_author, m_license, m_readme));
}

/*****************************************************************************/
std::string TargetMetadata::getMetadataFromString(const std::string& inString) const
{
	if (String::equals("version", inString))
		return m_versionString;

	if (m_version.hasMajor() && String::equals("versionMajor", inString))
		return std::to_string(m_version.major());

	if (m_version.hasMinor() && String::equals("versionMinor", inString))
		return std::to_string(m_version.minor());

	if (m_version.hasPatch() && String::equals("versionPatch", inString))
		return std::to_string(m_version.patch());

	if (m_version.hasTweak() && String::equals("versionTweak", inString))
		return std::to_string(m_version.tweak());

	if (String::equals("name", inString))
		return m_name;

	if (String::equals("description", inString))
		return m_description;

	if (String::equals("homepage", inString))
		return m_homepage;

	if (String::equals("author", inString))
		return m_author;

	if (String::equals("license", inString))
		return m_license;

	if (String::equals("readme", inString))
		return m_readme;

	return std::string();
}

}
