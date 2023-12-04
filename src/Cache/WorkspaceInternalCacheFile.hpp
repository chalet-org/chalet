/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Cache/ExternalDependencyCache.hpp"
#include "Compile/Strategy/StrategyType.hpp"

namespace chalet
{
struct WorkspaceCache;
struct SourceCache;
struct JsonFile;

struct WorkspaceInternalCacheFile
{
	using GetDataCallback = std::function<std::string()>;
	WorkspaceInternalCacheFile();
	CHALET_DISALLOW_COPY_MOVE(WorkspaceInternalCacheFile);
	~WorkspaceInternalCacheFile();

	bool loadExternalDependencies(const std::string& inPath);
	bool saveExternalDependencies();

	bool sourceCacheAvailable() const;
	bool setSourceCache(const std::string& inId, const StrategyType inStrategy);

	std::string getDataValue(const std::string& inHash, const GetDataCallback& onGet);
	std::string getDataValueFromPath(const std::string& inPath, const GetDataCallback& onGet);

	bool buildFileChanged() const noexcept;

	bool buildHashChanged() const noexcept;
	void setBuildHash(const std::string& inValue) noexcept;

	void setBuildOutputCache(const std::string& inPath, const std::string& inToolchain);

	void setForceRebuild(const bool inValue);

	bool canWipeBuildFolder() const;

	bool metadataChanged() const;
	void checkForMetadataChange(const std::string& inHash);

	bool themeChanged() const noexcept;
	void checkIfThemeChanged();

	bool appVersionChanged() const noexcept;
	void checkIfAppVersionChanged(const std::string& inAppPath);

	void addExtraHash(std::string&& inHash);

	SourceCache& sources() const;
	ExternalDependencyCache& externalDependencies();

private:
	friend struct WorkspaceCache;

	bool initialize(const std::string& inFilename, const std::string& inBuildFile);
	bool save();

	bool removeSourceCache(const std::string& inId);
	bool removeExtraCache(const std::string& inId);
	StringList getCacheIdsToNotRemove() const;

	void addToDataCache(const std::string& inKey, std::string inValue);
	const std::string& getDataCacheValue(const std::string& inKey);

	std::string getAppVersionHash(std::string appPath) const;

	StringList m_doNotRemoves;
	StringList m_extraHashes;

	Unique<JsonFile> m_dataFile;

	std::string m_filename;
	std::string m_hashBuild;
	std::string m_hashTheme;
	std::string m_hashMetadata;
	std::string m_hashVersion;
	std::string m_hashVersionDebug;

	std::time_t m_lastBuildFileWrite = 0;

	SourceCache* m_sources = nullptr;

	ExternalDependencyCache m_externalDependencies;
	HeapDictionary<SourceCache> m_sourceCaches;
	Dictionary<std::string> m_outputPathCache;
	Dictionary<std::string> m_dataCache;

	std::optional<bool> m_metadataChanged;

	bool m_forceRebuild = false;
	bool m_buildHashChanged = false;
	bool m_toolchainChangedForBuildOutputPath = false;
	bool m_buildFileChanged = false;
	bool m_themeChanged = false;
	bool m_appVersionChanged = false;
	bool m_dirty = false;
};
}
