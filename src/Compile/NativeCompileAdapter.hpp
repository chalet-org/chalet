/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;
struct SourceCache;
struct SourceTarget;

struct NativeCompileAdapter
{
	explicit NativeCompileAdapter(const BuildState& inState);

	void addChangedTarget(const SourceTarget& inProject);

	bool checkDependentTargets(const SourceTarget& inProject) const;
	bool anyCmakeOrSubChaletTargetsChanged() const;

	void setDependencyCacheSize(const size_t inSize);
	void clearDependencyCache();
	bool fileChangedOrDependentChanged(const std::string& source, const std::string& target, const std::string& dependency);

private:
	const BuildState& m_state;
	SourceCache& m_sourceCache;

	StringList m_targetsChanged;

	std::unordered_map<std::string, bool> m_dependencyCache;
};
}
