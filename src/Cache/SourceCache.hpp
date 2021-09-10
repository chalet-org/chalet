/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SOURCE_CACHE_HPP
#define CHALET_SOURCE_CACHE_HPP

#include "Cache/LastWrite.hpp"

namespace chalet
{
using SourceLastWriteMap = std::unordered_map<std::string, LastWrite>;

struct SourceCache
{
	explicit SourceCache(SourceLastWriteMap& inLastWrites, const std::time_t inLastBuildTime);

	bool dirty() const;
	std::string asString(const std::string& inId) const;
	bool updateInitializedTime(const std::time_t inTime = 0);

	void addLastWrite(std::string inFile, const std::string& inRaw);

	bool fileChangedOrDoesNotExist(const std::string& inFile) const;
	bool fileChangedOrDoesNotExist(const std::string& inFile, const std::string& inDependency) const;
	bool fileChangedOrDependantChanged(const std::string& inFile, const std::string& inDependency) const;

private:
	friend struct WorkspaceInternalCacheFile;

	void makeUpdate(const std::string& inFile, LastWrite& outFileData) const;
	void forceUpdate(const std::string& inFile, LastWrite& outFileData) const;
	LastWrite& getLastWrite(const std::string& inFile) const;

	SourceLastWriteMap& m_lastWrites;

	std::time_t m_initializedTime = 0;
	std::time_t m_lastBuildTime = 0;

	mutable bool m_dirty = false;
};
}

#endif // CHALET_SOURCE_FILE_CACHE_HPP
