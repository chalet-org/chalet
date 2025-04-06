/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/Distribution/DistTargetType.hpp"
#include "Utility/GlobMatch.hpp"

namespace chalet
{
class BuildState;

struct IDistTarget;
using DistTarget = Unique<IDistTarget>;

struct IDistTarget
{
	using IncludeMap = std::map<std::string, std::string>;

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
	bool resolveDependentTargets(std::string& outDepends, std::string& outPath, const char* inKey) const;
	bool replaceVariablesInPathList(StringList& outList) const;
	bool expandGlobPatternsInList(StringList& outList, GlobMatch inSettings) const;
	bool expandGlobPatternsInMap(IncludeMap& outMap, GlobMatch inSettings) const;
	bool processIncludeExceptions(IncludeMap& outMap) const;
	bool validateWorkingDirectory(std::string& outPath) const;

	const BuildState& m_state;

private:
	std::string m_name;
	std::string m_outputDescription;

	DistTargetType m_type = DistTargetType::Unknown;

	bool m_includeInDistribution = true;
};

using DistributionTargetList = std::vector<DistTarget>;
}
