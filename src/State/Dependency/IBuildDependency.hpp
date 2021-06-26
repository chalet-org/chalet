/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IBUILD_DEPENDENCY_HPP
#define CHALET_IBUILD_DEPENDENCY_HPP

#include "State/Dependency/BuildDependencyType.hpp"

namespace chalet
{
struct IBuildDependency;
struct StatePrototype;
using BuildDependency = std::unique_ptr<IBuildDependency>;

struct IBuildDependency
{
	explicit IBuildDependency(const StatePrototype& inPrototype, const BuildDependencyType inType);
	virtual ~IBuildDependency() = default;

	[[nodiscard]] static BuildDependency make(const BuildDependencyType inType, const StatePrototype& inPrototype);

	virtual bool validate() = 0;

	BuildDependencyType type() const noexcept;
	bool isGit() const noexcept;

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

protected:
	const StatePrototype& m_prototype;

private:
	std::string m_name;

	BuildDependencyType m_type;
};

using BuildDependencyList = std::vector<BuildDependency>;
}

#endif // CHALET_IBUILD_DEPENDENCY_HPP
