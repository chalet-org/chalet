/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Dependency/ArchiveDependency.hpp"

#include "Core/CommandLineInputs.hpp"

#include "State/CentralState.hpp"
#include "System/Files.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ArchiveDependency::ArchiveDependency(const CentralState& inCentralState) :
	IExternalDependency(inCentralState, ExternalDependencyType::Archive)
{
}

/*****************************************************************************/
bool ArchiveDependency::initialize()
{
	if (!m_centralState.replaceVariablesInString(m_url, this))
		return false;

	if (!parseDestination())
		return false;

	return true;
}

/*****************************************************************************/
bool ArchiveDependency::validate()
{
	if (m_url.empty())
	{
		Diagnostic::error("The archive url was blank for '{}'.", this->name());
		return false;
	}

	if (m_format == ArchiveFormat::Unknown)
	{
		Diagnostic::error("The archive url for '{}' expected a zip or tar, but is an unsupported format: {}", this->name(), m_url);
		return false;
	}

	return true;
}

/*****************************************************************************/
const std::string& ArchiveDependency::getHash() const
{
	if (m_hash.empty())
	{
		auto hashable = Hash::getHashableString(this->name(), m_url);
		m_hash = Hash::string(hashable);
	}

	return m_hash;
}

/*****************************************************************************/
const std::string& ArchiveDependency::url() const noexcept
{
	return m_url;
}

void ArchiveDependency::setUrl(std::string&& inValue) noexcept
{
	m_url = std::move(inValue);

	if (String::endsWith(".zip", m_url))
		m_format = ArchiveFormat::Zip;
	else if (String::endsWith({ ".tar", ".tar.gz" }, m_url))
		m_format = ArchiveFormat::Tar;
	else
		m_format = ArchiveFormat::Unknown;
}

/*****************************************************************************/
const std::string& ArchiveDependency::subdirectory() const noexcept
{
	return m_subdirectory;
}

void ArchiveDependency::setSubdirectory(std::string&& inValue) noexcept
{
	m_subdirectory = std::move(inValue);
}

/*****************************************************************************/
const std::string& ArchiveDependency::destination() const noexcept
{
	return m_destination;
}

/*****************************************************************************/
ArchiveFormat ArchiveDependency::format() const noexcept
{
	return m_format;
}

/*****************************************************************************/
bool ArchiveDependency::parseDestination()
{
	// LOG("url: ", m_url);
	// LOG("name: ", m_name);

	if (!m_destination.empty())
		return false;

	const auto& externalDir = m_centralState.inputs().externalDirectory();
	chalet_assert(!externalDir.empty(), "externalDir can't be blank.");

	auto& targetName = this->name();
	m_destination = fmt::format("{}/{}", externalDir, targetName);

	return true;
}
}
