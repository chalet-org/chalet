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
using BuildTarget = Unique<IBuildTarget>;

struct IBuildTarget
{
	explicit IBuildTarget(const BuildState& inState, const BuildTargetType inType);
	virtual ~IBuildTarget() = default;

	[[nodiscard]] static BuildTarget make(const BuildTargetType inType, BuildState& inState);

	virtual bool validate() = 0;

	virtual bool initialize();

	BuildTargetType type() const noexcept;
	bool isSources() const noexcept;
	bool isScript() const noexcept;
	bool isSubChalet() const noexcept;
	bool isCMake() const noexcept;

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

	const std::string& description() const noexcept;
	void setDescription(std::string&& inValue) noexcept;

	const StringList& runArguments() const noexcept;
	void addRunArguments(StringList&& inList);
	void addRunArgument(std::string&& inValue);

	const StringList& runDependencies() const noexcept;
	void addRunDependencies(StringList&& inList);
	void addRunDependency(std::string&& inValue);

	bool runTarget() const noexcept;
	void setRunTarget(const bool inValue) noexcept;

	bool includeInBuild() const noexcept;
	void setIncludeInBuild(const bool inValue);

protected:
	void replaceVariablesInPathList(StringList& outList);

	const BuildState& m_state;

private:
	StringList m_runArguments;
	StringList m_runDependencies;

	std::string m_name;
	std::string m_description;

	BuildTargetType m_type = BuildTargetType::Unknown;
	bool m_runTarget = false;
	bool m_includeInBuild = true;
};
}

#endif // CHALET_IBUILD_TARGET_HPP
