/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WORKSPACE_CACHE_HPP
#define CHALET_WORKSPACE_CACHE_HPP

#include "Cache/CacheType.hpp"
#include "Cache/WorkspaceInternalCacheFile.hpp"
#include "Settings/SettingsType.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct CommandLineInputs;
struct CentralState;
struct CompilerTools;
class BuildState;

struct WorkspaceCache
{
	WorkspaceCache() = default;

	bool createCacheFolder(const CacheType inCacheType);
	bool exists(const CacheType inCacheType = CacheType::Local) const;
	std::string getHashPath(const std::string& inIdentifier, const CacheType inCacheType) const;
	std::string getCachePath(const std::string& inIdentifier, const CacheType inCacheType) const;

	WorkspaceInternalCacheFile& file() noexcept;

	JsonFile& getSettings(const SettingsType inType) noexcept;
	const JsonFile& getSettings(const SettingsType inType) const noexcept;
	void saveSettings(const SettingsType inType);

	bool removeStaleProjectCaches();
	bool saveProjectCache(const CommandLineInputs& inInputs);
	bool settingsCreated() const noexcept;

private:
	friend class BuildState;
	friend struct CentralState;
	friend struct SettingsManager;

	const std::string& getCacheRef(const CacheType inCacheType) const;
	void removeCacheFolder(const CacheType inCacheType);

	bool initialize(const CommandLineInputs& inInputs);
	bool initializeSettings(const CommandLineInputs& inInputs);

	bool updateSettingsFromToolchain(const CommandLineInputs& inInputs, const CompilerTools& inToolchain);

	WorkspaceInternalCacheFile m_cacheFile;

	JsonFile m_localSettings;
	JsonFile m_globalSettings;

	std::string m_cacheFolderLocal;
	// std::string m_cacheFolderGlobal;

	bool m_settingsCreated = false;
	bool m_removeOldCacheFolder = false;
};
}

#endif // CHALET_WORKSPACE_CACHE_HPP
