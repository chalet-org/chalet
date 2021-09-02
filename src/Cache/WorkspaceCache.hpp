/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WORKSPACE_CACHE_HPP
#define CHALET_WORKSPACE_CACHE_HPP

#include "Cache/CacheType.hpp"
#include "Cache/WorkspaceInternalCacheFile.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Settings/SettingsType.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct CommandLineInputs;
struct StatePrototype;
class BuildState;

struct WorkspaceCache
{
	explicit WorkspaceCache(const CommandLineInputs& inInputs);

	bool createCacheFolder(const CacheType inCacheType);
	bool exists(const CacheType inCacheType = CacheType::Local) const;
	std::string getHashPath(const std::string& inIdentifier, const CacheType inCacheType) const;
	std::string getCachePath(const std::string& inIdentifier, const CacheType inCacheType) const;

	WorkspaceInternalCacheFile& file() noexcept;

	JsonFile& getSettings(const SettingsType inType) noexcept;
	void saveSettings(const SettingsType inType);

	bool removeStaleProjectCaches();

private:
	friend class BuildState;
	friend struct StatePrototype;
	friend struct SettingsManager;

	const std::string& getCacheRef(const CacheType inCacheType) const;
	void removeCacheFolder(const CacheType inCacheType);

	bool initialize();
	bool initializeSettings();

	const CommandLineInputs& m_inputs;

	WorkspaceInternalCacheFile m_cacheFile;

	JsonFile m_localSettings;
	JsonFile m_globalSettings;

	std::string m_cacheFolderLocal;
	// std::string m_cacheFolderGlobal;

	bool m_removeOldCacheFolder = false;
};
}

#endif // CHALET_WORKSPACE_CACHE_HPP
