/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IBUILD_TARGET_HPP
#define CHALET_IBUILD_TARGET_HPP

#include "State/Target/BuildTargetType.hpp"

namespace chalet
{
class BuildState;

struct IBuildTarget;
using BuildTarget = std::unique_ptr<IBuildTarget>;

struct IBuildTarget
{
	explicit IBuildTarget(const BuildState& inState, const BuildTargetType inType);
	virtual ~IBuildTarget() = default;

	[[nodiscard]] static BuildTarget make(const BuildTargetType inType, const BuildState& inState);

	virtual void initialize() = 0;
	virtual bool validate() = 0;

	BuildTargetType type() const noexcept;
	bool isProject() const noexcept;
	bool isScript() const noexcept;
	bool isSubChalet() const noexcept;
	bool isCMake() const noexcept;

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

	const std::string& description() const noexcept;
	void setDescription(std::string&& inValue) noexcept;

	bool includeInBuild() const noexcept;
	void setIncludeInBuild(const bool inValue);

protected:
	const BuildState& m_state;

private:
	std::string m_name;
	std::string m_description;

	BuildTargetType m_type;
	bool m_includeInBuild = true;
};

using BuildTargetList = std::vector<BuildTarget>;
}

#endif // CHALET_IBUILD_TARGET_HPP
