/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/BundleArchiveTarget.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/GlobMatch.hpp"
#include "Utility/List.hpp"

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

	return true;
}

/*****************************************************************************/
bool BundleArchiveTarget::validate()
{
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
	List::addIfDoesNotExist(m_includes, std::move(inValue));
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
