/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IDIST_TARGET_HPP
#define CHALET_IDIST_TARGET_HPP

#include "State/Distribution/DistTargetType.hpp"

namespace chalet
{
class BuildState;

struct IDistTarget;
using DistTarget = Unique<IDistTarget>;

struct IDistTarget
{
	explicit IDistTarget(const BuildState& inState, const DistTargetType inType);
	virtual ~IDistTarget() = default;

	[[nodiscard]] static DistTarget make(const DistTargetType inType, const BuildState& inState);

	virtual bool initialize() = 0;
	virtual bool validate() = 0;

	DistTargetType type() const noexcept;
	bool isScript() const noexcept;
	bool isProcess() const noexcept;
	bool isDistributionBundle() const noexcept;
	bool isArchive() const noexcept;
	bool isMacosDiskImage() const noexcept;

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

	const std::string& outputDescription() const noexcept;
	void setOutputDescription(std::string&& inValue) noexcept;

	bool includeInDistribution() const noexcept;
	void setIncludeInDistribution(const bool inValue);

protected:
	void replaceVariablesInPathList(StringList& outList);

	const BuildState& m_state;

private:
	std::string m_name;
	std::string m_outputDescription;

	DistTargetType m_type;

	bool m_includeInDistribution = true;
};

using DistributionTargetList = std::vector<DistTarget>;
}

#endif // CHALET_IDIST_TARGET_HPP
