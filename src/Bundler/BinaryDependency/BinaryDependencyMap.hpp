/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;

class BinaryDependencyMap
{
	using InnerMap = std::unordered_multimap<std::string, StringList>;

public:
	explicit BinaryDependencyMap(const BuildState& inState);

	bool getExecutableDependencies(const std::string& inPath, StringList& outList, StringList* outNotFound = nullptr);

	InnerMap::const_iterator begin() const;
	InnerMap::const_iterator end() const;

	void setIncludeWinUCRT(const bool inValue);

	void addExcludesFromList(const StringList& inList);

	void clearSearchDirs() noexcept;
	void addSearchDirsFromList(const StringList& inList);

	void log() const;

	void populateToList(StringList& outList, const StringList& inExclusions) const;
	bool gatherFromList(const StringList& inList, i32 levels = 2);

	const StringList& notCopied() const noexcept;

private:
	bool gatherDependenciesOf(const std::string& inPath, i32 levels);
	bool resolveDependencyPath(std::string& outDep);

	const BuildState& m_state;

	InnerMap m_map;
	StringList m_list;

	StringList m_excludes;
	StringList m_searchDirs;

	StringList m_notCopied;

	bool m_includeWinUCRT = true;
};
}
