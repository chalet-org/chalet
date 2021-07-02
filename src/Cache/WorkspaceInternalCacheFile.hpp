/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WORKSPACE_SOURCE_CACHE_FILE_HPP
#define CHALET_WORKSPACE_SOURCE_CACHE_FILE_HPP

#include "Cache/ExternalDependencyCache.hpp"
#include "Cache/SourceCache.hpp"

namespace chalet
{
struct WorkspaceCache;

struct WorkspaceInternalCacheFile
{
	explicit WorkspaceInternalCacheFile(WorkspaceCache& inCache);

	const std::string& hashStrategy() const noexcept;
	void setHashStrategy(std::string&& inValue) noexcept;

	bool initialize(const std::string& inFilename, const std::string& inBuildFile);
	bool save();

	bool loadExternalDependencies(const std::string& inPath);
	bool saveExternalDependencies();

	bool setSourceCache(const std::string& inId, const bool inNative = false);
	bool removeSourceCache(const std::string& inId);
	bool removeExtraCache(const std::string& inId);

	bool buildFileChanged() const noexcept;

	bool themeChanged() const noexcept;
	void checkIfThemeChanged();

	bool appVersionChanged() const noexcept;
	void checkIfAppVersionChanged(const std::string& inAppPath);

	void addExtraHash(std::string&& inHash);

	SourceCache& sources() const;
	ExternalDependencyCache& externalDependencies();

	StringList getCacheIdsForRemoval() const;

private:
	std::string getAppVersionHash(std::string appPath);

	WorkspaceCache& m_cache;

	StringList m_doNotRemoves;
	StringList m_extraHashes;

	std::string m_filename;
	std::string m_hashStrategy;
	std::string m_hashTheme;
	std::string m_hashVersion;
	std::string m_hashVersionDebug;

	std::time_t m_initializedTime = 0;
	std::time_t m_lastBuildFileWrite = 0;

	SourceCache* m_sources = nullptr;

	ExternalDependencyCache m_externalDependencies;
	std::string m_externalDependencyCachePath;
	mutable std::unordered_map<std::string, std::unique_ptr<SourceCache>> m_sourceCaches;
	SourceLastWriteMap m_lastWrites;

	bool m_buildFileChanged = false;
	bool m_themeChanged = false;
	bool m_appVersionChanged = false;
	bool m_dirty = false;
};
}

#endif // CHALET_WORKSPACE_SOURCE_CACHE_FILE_HPP
