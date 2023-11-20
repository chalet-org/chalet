/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/BundleArchiveTarget.hpp"

#include "State/AncillaryTools.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "System/Files.hpp"
#include "Utility/GlobMatch.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
Dictionary<ArchiveFormat> getArchiveFormats()
{
	return {
		{ "zip", ArchiveFormat::Zip },
		{ "tar", ArchiveFormat::Tar },
	};
}
}

/*****************************************************************************/
BundleArchiveTarget::BundleArchiveTarget(const BuildState& inState) :
	IDistTarget(inState, DistTargetType::BundleArchive)
{
}

/*****************************************************************************/
bool BundleArchiveTarget::initialize()
{
	if (!replaceVariablesInPathList(m_includes))
		return false;

#if defined(CHALET_MACOS)
	if (!m_state.replaceVariablesInString(m_macosNotarizationProfile, this))
		return false;
#endif

	return true;
}

/*****************************************************************************/
bool BundleArchiveTarget::validate()
{
#if defined(CHALET_MACOS)
	if (!m_macosNotarizationProfile.empty())
	{
		auto xcodeVersion = m_state.tools.xcodeVersionMajor();
		if (xcodeVersion < 13)
		{
			Diagnostic::warn("Notarization using 'macosNotarizationProfile' requires Xcode 13 or higher, but found: {}", xcodeVersion);
			m_macosNotarizationProfile.clear();
		}
	}
#endif

	return true;
}

/*****************************************************************************/
std::string BundleArchiveTarget::getOutputFilename(const std::string& inBaseName) const
{
	if (m_format == ArchiveFormat::Tar)
		return fmt::format("{}.tar.gz", inBaseName);
	else
		return fmt::format("{}.zip", inBaseName);
}

/*****************************************************************************/
const StringList& BundleArchiveTarget::includes() const noexcept
{
	return m_includes;
}

void BundleArchiveTarget::addIncludes(StringList&& inList)
{
	List::forEach(inList, this, &BundleArchiveTarget::addInclude);
}

void BundleArchiveTarget::addInclude(std::string&& inValue)
{
	if (String::endsWith('/', inValue))
		inValue.pop_back();

	List::addIfDoesNotExist(m_includes, std::move(inValue));
}

/*****************************************************************************/
const std::string& BundleArchiveTarget::macosNotarizationProfile() const noexcept
{
	return m_macosNotarizationProfile;
}

void BundleArchiveTarget::setMacosNotarizationProfile(std::string&& inValue)
{
	m_macosNotarizationProfile = std::move(inValue);
}

/*****************************************************************************/
ArchiveFormat BundleArchiveTarget::format() const noexcept
{
	return m_format;
}

void BundleArchiveTarget::setFormat(std::string&& inValue)
{
	m_format = getFormatFromString(inValue);
}

/*****************************************************************************/
ArchiveFormat BundleArchiveTarget::getFormatFromString(const std::string& inValue) const
{
	auto formats = getArchiveFormats();
	if (formats.find(inValue) != formats.end())
	{
		return formats.at(inValue);
	}

	return ArchiveFormat::Zip;
}
}
