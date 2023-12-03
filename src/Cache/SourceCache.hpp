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

	void setLastBuildStrategy(const StrategyType inValue, const bool inCheckChanges = false) noexcept;
	void setLastBuildStrategy(const i32 inValue, const bool inCheckChanges = false) noexcept;
	bool buildStrategyChanged() const noexcept;
	bool canRemoveCachedFolder() const noexcept;

	void addVersion(const std::string& inExecutable, const std::string& inValue);
	void addExternalRebuild(const std::string& inPath, const std::string& inValue);

	bool dirty() const;
	Json asJson() const;

	bool updateInitializedTime();
	bool isNewBuild() const;

	bool fileChangedOrDoesNotExist(const std::string& inFile) const;
	bool fileChangedOrDoesNotExist(const std::string& inFile, const std::string& inDependency) const;
	bool fileChangedOrDoesNotExistWithCache(const std::string& inFile);

	bool versionRequriesUpdate(const std::string& inFile, std::string& outExistingValue);
	bool externalRequiresRebuild(const std::string& inPath);

	void updateFileCache(const std::string& inFile, const bool inResult);

private:
	friend struct WorkspaceInternalCacheFile;

	const std::string& dataCache(const std::string& inMainKey, const char* inKey) noexcept;
	void addDataCache(const std::string& inMainKey, const char* inKey, const std::string& inValue);
	void addToFileCache(size_t inValue);

	Dictionary<Dictionary<std::string>> m_dataCache;

	std::vector<size_t> m_fileCache;

	std::time_t m_initializedTime = 0;
	std::time_t m_lastBuildTime = 0;

	StrategyType m_lastBuildStrategy = StrategyType::None;
	bool m_buildStrategyChanged = false;
	mutable bool m_dirty = false;
};
}
