/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Strategy/StrategyType.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
struct SourceCache
{
	explicit SourceCache(const std::time_t inLastBuildTime);

	bool dirty() const;
	Json asJson() const;

	bool buildStrategyChanged() const noexcept;

	void addDataCache(const std::string& inKey, const std::string& inValue);
	void addDataCache(const std::string& inKey, std::string&& inValue);
	void addDataCache(const std::string& inKey, const bool inValue);

	bool dataCacheValueIsFalse(const std::string& inHash);

	bool fileChangedOrDoesNotExist(const std::string& inFile) const;
	bool fileChangedOrDoesNotExist(const std::string& inFile, const std::string& inDependency) const;
	bool fileChangedOrDoesNotExistWithCache(const std::string& inFile);

	void addOrRemoveFileCache(const std::string& inFile, const bool inResult);

private:
	friend struct WorkspaceInternalCacheFile;

	bool updateInitializedTime();
	void setLastBuildStrategy(const i32 inValue, const bool inCheckChanges = false) noexcept;

	bool canRemoveCachedFolder() const noexcept;
	const std::string& getDataCacheValue(const std::string& inKey) noexcept;
	void addToFileCache(size_t inValue);

	Dictionary<std::string> m_dataCache;

	std::vector<size_t> m_fileCache;

	std::time_t m_initializedTime = 0;
	std::time_t m_lastBuildTime = 0;

	StrategyType m_lastBuildStrategy = StrategyType::None;
	bool m_buildStrategyChanged = false;
	mutable bool m_dirty = false;
};
}
