/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WORKSPACE_SOURCE_CACHE_FILE_HPP
#define CHALET_WORKSPACE_SOURCE_CACHE_FILE_HPP

#include "Cache/SourceCache.hpp"

namespace chalet
{
struct WorkspaceInternalCacheFile
{
	const std::string& hashStrategy() const noexcept;
	void setHashStrategy(std::string&& inValue) noexcept;

	bool initialize(const std::string& inFilename);
	bool save();

	bool setSourceCache(const std::string& inId);
	bool removeSourceCache(const std::string& inId);

	bool workingDirectoryChanged() const noexcept;
	void checkIfWorkingDirectoryChanged(const std::string& inWorkingDirectory);

	bool appVersionChanged() const noexcept;
	void checkIfAppVersionChanged(const std::string& inAppPath);

	void addExtraHash(std::string&& inHash);

	SourceCache& sources() const;

	StringList getCacheIds() const;

private:
	std::string getAppVersionHash(std::string appPath);

	StringList m_extraHashes;

	std::string m_filename;
	std::string m_hashStrategy;
	std::string m_hashVersion;
	std::string m_hashVersionDebug;
	std::string m_hashWorkingDirectory;

	std::time_t m_lastBuildTime = 0;
	std::time_t m_initializedTime = 0;

	SourceCache* m_sources = nullptr;

	mutable std::unordered_map<std::string, std::unique_ptr<SourceCache>> m_sourceCaches;

	bool m_workingDirectoryChanged = false;
	bool m_appVersionChanged = false;
	bool m_dirty = false;
};
}

#endif // CHALET_WORKSPACE_SOURCE_CACHE_FILE_HPP
