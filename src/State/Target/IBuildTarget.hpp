/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IBUILD_TARGET_HPP
#define CHALET_IBUILD_TARGET_HPP

#include "State/Target/TargetType.hpp"

namespace chalet
{
class BuildState;

struct IBuildTarget
{
	explicit IBuildTarget(const BuildState& inState, const TargetType inType);
	virtual ~IBuildTarget() = default;

	[[nodiscard]] static std::unique_ptr<IBuildTarget> make(const BuildState& inState, const TargetType inType);

	TargetType type() const noexcept;
	bool isProject() const noexcept;
	bool isScript() const noexcept;
	bool isCMake() const noexcept;

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

	const std::string& description() const noexcept;
	void setDescription(const std::string& inValue) noexcept;

	bool includeInBuild() const noexcept;
	void setIncludeInBuild(const bool inValue);

protected:
	void parseStringVariables(std::string& outString);

	const BuildState& m_state;

private:
	std::string m_name;
	std::string m_description;

	TargetType m_type;
	bool m_includeInBuild = true;
};

using BuildTargetList = std::vector<std::unique_ptr<IBuildTarget>>;
}

#endif // CHALET_IBUILD_TARGET_HPP
