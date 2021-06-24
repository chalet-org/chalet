/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WORKSPACE_CACHE_HPP
#define CHALET_WORKSPACE_CACHE_HPP

#include "Cache/WorkspaceInternalCacheFile.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct CommandLineInputs;
struct StatePrototype;
class BuildState;

struct WorkspaceCache
{
	enum class Type
	{
		Local,
		Global
	};

	explicit WorkspaceCache(const CommandLineInputs& inInputs);

	bool createCacheFolder(const Type inCacheType);
	bool exists(const Type inCacheType = Type::Local) const;
	std::string getHashPath(const std::string& inIdentifier, const Type inCacheType) const;
	std::string getCachePath(const std::string& inFolder, const Type inCacheType) const;
	void setWorkspaceHash(const std::string& inToHash) noexcept;

	WorkspaceInternalCacheFile& file() noexcept;

	JsonFile& localConfig() noexcept;
	void saveLocalConfig();

	JsonFile& globalConfig() noexcept;
	void saveGlobalConfig();

	void removeBuildIfCacheChanged(const std::string& inBuildDir);
	bool removeStaleProjectCaches();

private:
	friend class BuildState;
	friend struct StatePrototype;
	friend struct ConfigManager;

	const std::string& getCacheRef(const Type inCacheType) const;
	void removeCacheFolder(const Type inCacheType);

	bool initialize();

	const CommandLineInputs& m_inputs;

	WorkspaceInternalCacheFile m_cacheFile;

	JsonFile m_localConfig;
	JsonFile m_globalConfig;

	std::string m_cacheFolderLocal;
	// std::string m_cacheFolderGlobal;

	std::size_t m_workspaceHash = 0;

	bool m_removeOldCacheFolder = false;
};
}

#endif // CHALET_WORKSPACE_CACHE_HPP
