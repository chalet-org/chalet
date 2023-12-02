/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/Target/BuildTargetType.hpp"
#include "Utility/GlobMatch.hpp"

namespace chalet
{
class BuildState;

struct IBuildTarget;
using BuildTarget = Unique<IBuildTarget>;

struct IBuildTarget
{
	explicit IBuildTarget(const BuildState& inState, const BuildTargetType inType);
	virtual ~IBuildTarget() = default;

	[[nodiscard]] static BuildTarget make(const BuildTargetType inType, const BuildState& inState);

	virtual bool validate() = 0;
	virtual const std::string& getHash() const = 0;

	virtual bool initialize();

	BuildTargetType type() const noexcept;
	bool isSources() const noexcept;
	bool isSubChalet() const noexcept;
	bool isCMake() const noexcept;
	bool isScript() const noexcept;
	bool isProcess() const noexcept;
	bool isValidation() const noexcept;

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

	const std::string& outputDescription() const noexcept;
	void setOutputDescription(std::string&& inValue) noexcept;

	bool includeInBuild() const noexcept;
	void setIncludeInBuild(const bool inValue);

protected:
	bool replaceVariablesInPathList(StringList& outList) const;
	bool expandGlobPatternsInList(StringList& outList, GlobMatch inSettings) const;

	const BuildState& m_state;

	mutable std::string m_hash;

private:
	std::string m_name;
	std::string m_outputDescription;

	BuildTargetType m_type = BuildTargetType::Unknown;
	bool m_includeInBuild = true;
};
}
