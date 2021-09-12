/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SOURCE_CACHE_HPP
#define CHALET_SOURCE_CACHE_HPP

#include "Cache/LastWrite.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
using SourceLastWriteMap = std::unordered_map<std::string, LastWrite>;

struct SourceCache
{
	explicit SourceCache(const std::time_t inLastBuildTime);

	bool native() const noexcept;
	void setNative(const bool inValue) noexcept;

	bool dirty() const;
	Json asJson(const std::string& kKeyBuildLastBuilt, const std::string& kKeyBuildNative, const std::string& kKeyBuildFiles) const;

	bool updateInitializedTime(const std::time_t inTime = 0);

	void addLastWrite(std::string inFile, const std::string& inRaw);
	void addLastWrite(std::string inFile, const std::time_t inLastWrite);

	bool fileChangedOrDoesNotExist(const std::string& inFile) const;
	bool fileChangedOrDoesNotExist(const std::string& inFile, const std::string& inDependency) const;
	bool fileChangedOrDependantChanged(const std::string& inFile, const std::string& inDependency) const;

private:
	friend struct WorkspaceInternalCacheFile;

	void makeUpdate(const std::string& inFile, LastWrite& outFileData) const;
	void forceUpdate(const std::string& inFile, LastWrite& outFileData) const;
	LastWrite& getLastWrite(const std::string& inFile) const;

	mutable SourceLastWriteMap m_lastWrites;

	std::time_t m_initializedTime = 0;
	std::time_t m_lastBuildTime = 0;

	bool m_native = false;
	mutable bool m_dirty = false;
};
}

#endif // CHALET_SOURCE_FILE_CACHE_HPP
