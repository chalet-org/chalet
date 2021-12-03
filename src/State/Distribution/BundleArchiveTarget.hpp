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
	explicit BundleArchiveTarget();

	virtual bool initialize(const BuildState& inState) final;
	virtual bool validate() final;

	const StringList& targets() const noexcept;
	void addTargets(StringList&& inList);
	void addTarget(std::string&& inValue);

private:
	StringList m_targets;
};
}

#endif // CHALET_BUNDLE_ARCHIVE_TARGET_HPP
