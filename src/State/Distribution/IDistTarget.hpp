/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

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
	bool isDistributionBundle() const noexcept;
	bool isArchive() const noexcept;
	bool isMacosDiskImage() const noexcept;
	bool isScript() const noexcept;
	bool isProcess() const noexcept;
	bool isValidation() const noexcept;

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

	const std::string& outputDescription() const noexcept;
	void setOutputDescription(std::string&& inValue) noexcept;

	bool includeInDistribution() const noexcept;
	void setIncludeInDistribution(const bool inValue);

protected:
	bool replaceVariablesInPathList(StringList& outList) const;
	bool processEachPathList(StringList inList, std::function<bool(std::string&& inValue)> onProcess) const;
	bool processIncludeExceptions(StringList& outList) const;

	const BuildState& m_state;

private:
	std::string m_name;
	std::string m_outputDescription;

	DistTargetType m_type = DistTargetType::Unknown;

	bool m_includeInDistribution = true;
};

using DistributionTargetList = std::vector<DistTarget>;
}
