/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WORKSPACE_SOURCE_CACHE_FILE_HPP
#define CHALET_WORKSPACE_SOURCE_CACHE_FILE_HPP

#include "Cache/ExternalDependencyCache.hpp"

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

	bool setSourceCache(const std::string& inId, const bool inNative = false);
	bool removeSourceCache(const std::string& inId);
	bool removeExtraCache(const std::string& inId);

	bool buildHashChanged() const noexcept;
	bool buildFileChanged() const noexcept;

	bool themeChanged() const noexcept;
	void checkIfThemeChanged();

	bool appVersionChanged() const noexcept;
	void checkIfAppVersionChanged(const std::string& inAppPath);

	void addExtraHash(std::string&& inHash);

	void setDisallowSave(const bool inValue);

	SourceCache& sources() const;
	ExternalDependencyCache& externalDependencies();

	StringList getCacheIdsForRemoval() const;

private:
	void setBuildHash(const std::string& inValue) noexcept;

	std::string getAppVersionHash(const std::string& inAppPath);

	StringList m_doNotRemoves;
	StringList m_extraHashes;

	Unique<JsonFile> m_dataFile;

	std::string m_filename;
	std::string m_buildHash;
	std::string m_hashTheme;
	std::string m_hashVersion;
	std::string m_hashVersionDebug;

	std::time_t m_initializedTime = 0;
	std::time_t m_lastBuildFileWrite = 0;

	SourceCache* m_sources = nullptr;

	ExternalDependencyCache m_externalDependencies;
	std::string m_externalDependencyCachePath;
	HeapDictionary<SourceCache> m_sourceCaches;

	const std::string kKeyHashes;
	const std::string kKeyHashBuild;
	const std::string kKeyHashTheme;
	const std::string kKeyHashVersionDebug;
	const std::string kKeyHashVersionRelease;
	const std::string kKeyHashExtra;
	const std::string kKeyLastChaletJsonWriteTime;
	const std::string kKeyBuilds;
	const std::string kKeyBuildLastBuilt;
	const std::string kKeyBuildNative;
	const std::string kKeyBuildFiles;
	const std::string kKeyDataCache;

	bool m_buildHashChanged = false;
	bool m_buildFileChanged = false;
	bool m_themeChanged = false;
	bool m_appVersionChanged = false;
	bool m_disallowSave = false;
	bool m_dirty = false;
};
}

#endif // CHALET_WORKSPACE_SOURCE_CACHE_FILE_HPP
