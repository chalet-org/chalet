/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IBUILD_DEPENDENCY_HPP
#define CHALET_IBUILD_DEPENDENCY_HPP

#include "State/Dependency/BuildDependencyType.hpp"

namespace chalet
{
class BuildState;

struct IBuildDependency
{
	explicit IBuildDependency(const BuildState& inState, const BuildDependencyType inType);
	virtual ~IBuildDependency() = default;

	[[nodiscard]] static std::unique_ptr<IBuildDependency> make(const BuildState& inState, const BuildDependencyType inType);

	BuildDependencyType type() const noexcept;
	bool isGit() const noexcept;

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

protected:
	const BuildState& m_state;

private:
	std::string m_name;

	BuildDependencyType m_type;
};

using BuildDependencyList = std::vector<std::unique_ptr<IBuildDependency>>;
}

#endif // CHALET_IBUILD_DEPENDENCY_HPP
