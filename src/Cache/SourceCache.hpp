/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Cache/LastWrite.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
using SourceLastWriteMap = Dictionary<LastWrite>;

struct SourceCache
{
	explicit SourceCache(const std::time_t inLastBuildTime);

	void setLastBuildStrategy(const StrategyType inValue, const bool inCheckChanges = false) noexcept;
	void setLastBuildStrategy(const i32 inValue, const bool inCheckChanges = false) noexcept;
	bool buildStrategyChanged() const noexcept;
	bool canRemoveCachedFolder() const noexcept;

	void addVersion(const std::string& inExecutable, const std::string& inValue);
	void addArch(const std::string& inExecutable, const std::string& inValue);
	void addExternalRebuild(const std::string& inPath, const std::string& inValue);

	bool dirty() const;
	Json asJson() const;

	bool targetHashChanged(const std::string& inTargetName, const std::string& inTargetHash);

	bool updateInitializedTime();
	bool isNewBuild() const;

	void addLastWrite(std::string inFile, const std::string& inRaw);
	void addLastWrite(std::string inFile, const std::time_t inLastWrite);

	bool fileChangedOrDoesNotExist(const std::string& inFile) const;
	bool fileChangedOrDoesNotExist(const std::string& inFile, const std::string& inDependency) const;
	bool fileChangedOrDependantChanged(const std::string& inFile, const std::string& inDependency) const;

	bool versionRequriesUpdate(const std::string& inFile, std::string& outExistingValue);
	bool archRequriesUpdate(const std::string& inFile, std::string& outExistingValue);
	bool externalRequiresRebuild(const std::string& inPath);

	void markForLater(const std::string& inFile);

private:
	friend struct WorkspaceInternalCacheFile;

	const std::string& dataCache(const std::string& inMainKey, const char* inKey) noexcept;
	void addDataCache(const std::string& inMainKey, const char* inKey, const std::string& inValue);

	void makeUpdate(const std::string& inFile, LastWrite& outFileData) const;
	LastWrite& getLastWrite(const std::string& inFile) const;

	Dictionary<Dictionary<std::string>> m_dataCache;

	mutable SourceLastWriteMap m_lastWrites;

	std::time_t m_initializedTime = 0;
	std::time_t m_lastBuildTime = 0;

	StrategyType m_lastBuildStrategy = StrategyType::None;
	bool m_buildStrategyChanged = false;
	mutable bool m_dirty = false;
};
}
