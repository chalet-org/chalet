/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WORKSPACE_CACHE_HPP
#define CHALET_WORKSPACE_CACHE_HPP

#include "Core/CommandLineInputs.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct CommandLineInputs;
class BuildState;
struct StatePrototype;

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
	std::string getHash(std::size_t inWorkspaceHash, const std::string& inIdentifier, const Type inCacheType) const;
	std::string getPath(const std::string& inFolder, const Type inCacheType) const;
	std::string getCacheKey(const std::string& inName, const std::string& inConfig);

	JsonFile& localConfig() noexcept;
	void saveLocalConfig();

	JsonFile& globalConfig() noexcept;
	void saveGlobalConfig();

	bool appBuildChanged() const noexcept;
	void checkIfCompileStrategyChanged(const std::string& inToolchain);
	void checkIfWorkingDirectoryChanged();
	void addSourceCache(const std::string& inHash);

	void removeStaleProjectCaches(const std::string& inToolchain, const Type inCacheType);
	void removeBuildIfCacheChanged(const std::string& inBuildDir);

private:
	friend class BuildState;
	friend struct StatePrototype;

	const std::string& getCacheRef(const Type inCacheType) const;
	void removeCacheFolder(const Type inCacheType);

	void initialize(const std::string& inAppPath);
	void makeAppVersionCheck(const std::string& inAppPath);
	std::string getBuildHash(std::string appPath);

	bool removeUnusedProjectFiles(const StringList& inHashes, const Type inCacheType);

	const CommandLineInputs& m_inputs;

	JsonFile m_localConfig;
	JsonFile m_globalConfig;

	const std::string kKeySettings{ "settings" };
	const std::string kKeyStrategy{ "strategy" };
	const std::string kKeyWorkingDirectory{ "workingDirectory" };
	const std::string kKeyData{ "data" };

	const std::string kKeyDataVersion{ "01" };
	const std::string kKeyDataVersionDebug{ "f1" };
	const std::string kKeyDataWorkingDirectory{ "02" };
	const std::string kKeyDataStrategy{ "03" };
	const std::string kKeyDataTargetArchitecture{ "04" };
	const std::string kKeyDataSourceList{ "05" };

	std::string m_cacheLocal;
	std::string m_cacheGlobal;

	bool m_appBuildChanged = false;
	bool m_compileStrategyChanged = false;
	bool m_targetArchitectureChanged = false;
	bool m_workingDirectoryChanged = false;
	bool m_removeOldCacheFolder = false;
};
}

#endif // CHALET_WORKSPACE_CACHE_HPP
