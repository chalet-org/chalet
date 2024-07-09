/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/ArchiveFormat.hpp"
#include "State/Distribution/IDistTarget.hpp"

namespace chalet
{
struct BundleArchiveTarget final : public IDistTarget
{
	explicit BundleArchiveTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;

	std::string getOutputFilename(const std::string& inBaseName) const;

	const IncludeMap& includes() const noexcept;
	void addIncludes(StringList&& inList);
	void addInclude(std::string&& inValue);
	void addInclude(const std::string& inKey, std::string&& inValue);

	const std::string& macosNotarizationProfile() const noexcept;
	void setMacosNotarizationProfile(std::string&& inValue);

	ArchiveFormat format() const noexcept;
	void setFormat(std::string&& inValue);

private:
	ArchiveFormat getFormatFromString(const std::string& inValue) const;

	IncludeMap m_includes;

	std::string m_macosNotarizationProfile;

	ArchiveFormat m_format = ArchiveFormat::Zip;
};
}
