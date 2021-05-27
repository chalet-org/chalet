/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_CACHE_HPP
#define CHALET_BUILD_CACHE_HPP

#include "BuildJson/WorkspaceInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/CommandLineInputs.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct BuildCache
{
	enum class Type
	{
		Local,
		Global
	};

	explicit BuildCache(const WorkspaceInfo& inInfo, const BuildPaths& inPaths);

	bool createCacheFolder(const Type inCacheType);
	bool exists(const Type inCacheType = Type::Local) const;
	std::string getHash(const std::string& inIdentifier, const Type inCacheType) const;
	std::string getPath(const std::string& inFolder, const Type inCacheType) const;

	JsonFile& environmentCache() noexcept;
	void saveEnvironmentCache();

	bool dirty() const noexcept;
	void setDirty(const bool inValue) noexcept;

	bool appBuildChanged() const noexcept;
	void checkIfCompileStrategyChanged();
	void checkIfTargetArchitectureChanged();
	void checkIfWorkingDirectoryChanged();

	void removeStaleProjectCaches(const std::string& inBuildConfig, const StringList& inProjectNames, const Type inCacheType);
	void removeBuildIfCacheChanged(const std::string& inBuildDir);

private:
	friend class BuildState;

	const std::string& getCacheRef(const Type inCacheType) const;
	void removeCacheFolder(const Type inCacheType);

	void initialize(const std::string& inAppPath);
	void makeAppVersionCheck(const std::string& inAppPath);
	std::string getBuildHash(std::string appPath);

	bool removeUnusedProjectFiles(const StringList& inHashes, const Type inCacheType);

	const WorkspaceInfo& m_info;
	const BuildPaths& m_paths;

	JsonFile m_environmentCache;

	const std::string kKeySettings{ "settings" };
	const std::string kKeyStrategy{ "strategy" };
	const std::string kKeyTargetArchitecture{ "targetArchitecture" };
	const std::string kKeyWorkingDirectory{ "workingDirectory" };
	const std::string kKeyData{ "data" };

	const std::string kKeyDataVersion{ "01" };
	const std::string kKeyDataVersionDebug{ "f1" };
	const std::string kKeyDataWorkingDirectory{ "02" };
	const std::string kKeyDataStrategy{ "03" };
	const std::string kKeyDataTargetArchitecture{ "04" };

	std::string m_cacheLocal;
	std::string m_cacheGlobal;

	bool m_dirty = false;
	bool m_appBuildChanged = false;
	bool m_compileStrategyChanged = false;
	bool m_targetArchitectureChanged = false;
	bool m_workingDirectoryChanged = false;
	bool m_removeOldCacheFolder = false;
};
}

#endif // CHALET_BUILD_CACHE_HPP
