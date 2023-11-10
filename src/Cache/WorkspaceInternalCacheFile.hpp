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
	WorkspaceInternalCacheFile();
	CHALET_DISALLOW_COPY_MOVE(WorkspaceInternalCacheFile);
	~WorkspaceInternalCacheFile();

	bool initialize(const std::string& inFilename, const std::string& inBuildFile);
	bool save();

	bool loadExternalDependencies(const std::string& inPath);
	bool saveExternalDependencies();

	bool sourceCacheAvailable() const;
	bool setSourceCache(const std::string& inId, const StrategyType inStrategy);
	bool removeSourceCache(const std::string& inId);
	bool removeExtraCache(const std::string& inId);

	bool buildHashChanged() const noexcept;
	bool buildFileChanged() const noexcept;
	void setBuildHash(const std::string& inValue, const bool inTemp) noexcept;

	bool forceRebuild() const noexcept;
	void setForceRebuild(const bool inValue);

	bool buildStrategyChanged() const;

	void checkForMetadataChange(const std::string& inHash);
	bool metadataChanged() const;

	bool themeChanged() const noexcept;
	void checkIfThemeChanged();

	bool appVersionChanged() const noexcept;
	void checkIfAppVersionChanged(const std::string& inAppPath);

	void addExtraHash(std::string&& inHash);

	void setDisallowSave(const bool inValue);

	SourceCache& sources() const;
	ExternalDependencyCache& externalDependencies();

	StringList getCacheIdsToNotRemove() const;

private:
	std::string getAppVersionHash(std::string appPath) const;

	StringList m_doNotRemoves;
	StringList m_extraHashes;

	Unique<JsonFile> m_dataFile;

	std::string m_filename;
	std::string m_tempBuildHash;
	std::string m_buildHash;
	std::string m_hashTheme;
	std::string m_hashMetadata;
	std::string m_hashVersion;
	std::string m_hashVersionDebug;

	std::time_t m_initializedTime = 0;
	std::time_t m_lastBuildFileWrite = 0;

	SourceCache* m_sources = nullptr;

	ExternalDependencyCache m_externalDependencies;
	HeapDictionary<SourceCache> m_sourceCaches;

	std::optional<bool> m_metadataChanged;

	bool m_forceRebuild = false;
	bool m_buildHashChanged = false;
	bool m_buildFileChanged = false;
	bool m_themeChanged = false;
	bool m_appVersionChanged = false;
	bool m_disallowSave = false;
	bool m_dirty = false;
};
}
