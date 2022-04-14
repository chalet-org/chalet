/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUNDLE_ARCHIVE_TARGET_HPP
#define CHALET_BUNDLE_ARCHIVE_TARGET_HPP

#include "State/Distribution/IDistTarget.hpp"

namespace chalet
{
struct BundleArchiveTarget final : public IDistTarget
{
	explicit BundleArchiveTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;

	const StringList& includes() const noexcept;
	void addIncludes(StringList&& inList);
	void addInclude(std::string&& inValue);

private:
	StringList m_includes;
};
}

#endif // CHALET_BUNDLE_ARCHIVE_TARGET_HPP
