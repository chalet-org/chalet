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

	[[nodiscard]] static BuildTarget make(const BuildTargetType inType, const BuildState& inState);

	virtual bool validate() = 0;

	virtual bool initialize();

	BuildTargetType type() const noexcept;
	bool isSources() const noexcept;
	bool isScript() const noexcept;
	bool isSubChalet() const noexcept;
	bool isCMake() const noexcept;
	bool isProcess() const noexcept;

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

	const std::string& outputDescription() const noexcept;
	void setOutputDescription(std::string&& inValue) noexcept;

	const StringList& copyFilesOnRun() const noexcept;
	void addCopyFilesOnRun(StringList&& inList);
	void addCopyFileOnRun(std::string&& inValue);

	bool includeInBuild() const noexcept;
	void setIncludeInBuild(const bool inValue);

	StringList getResolvedRunDependenciesList() const;

protected:
	bool replaceVariablesInPathList(StringList& outList) const;
	bool processEachPathList(StringList inList, std::function<void(std::string&& inValue)> onProcess) const;

	const BuildState& m_state;

private:
	StringList m_copyFilesOnRun;

	std::string m_name;
	std::string m_outputDescription;

	BuildTargetType m_type = BuildTargetType::Unknown;
	bool m_includeInBuild = true;
};
}

#endif // CHALET_IBUILD_TARGET_HPP
