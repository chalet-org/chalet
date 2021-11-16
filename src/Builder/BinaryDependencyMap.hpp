/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BINARY_DEPENDENCY_MAP_HPP
#define CHALET_BINARY_DEPENDENCY_MAP_HPP

namespace chalet
{
struct AncillaryTools;

class BinaryDependencyMap
{
	using InnerMap = std::unordered_multimap<std::string, StringList>;

public:
	explicit BinaryDependencyMap(const AncillaryTools& inTools);

	InnerMap::const_iterator begin() const;
	InnerMap::const_iterator end() const;

	void addExcludesFromList(const StringList& inList);

	void clearSearchDirs() noexcept;
	void addSearchDirsFromList(const StringList& inList);

	void log() const;

	void populateToList(StringList& outList, const StringList& inExclusions) const;
	bool gatherFromList(const StringList& inList, int levels = 2);

private:
	bool gatherDependenciesOf(const std::string& inPath, int levels);
	bool resolveDependencyPath(std::string& outDep) const;
	bool getExecutableDependencies(const std::string& inPath, StringList& outList) const;

	const AncillaryTools& m_tools;

	InnerMap m_map;
	StringList m_list;

	StringList m_excludes;
	StringList m_searchDirs;
};
}

#endif // CHALET_BINARY_DEPENDENCY_MAP_HPP
