/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/WorkspaceCache.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
WorkspaceCache::WorkspaceCache(const CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
	m_localConfig.load(fmt::format("{}/chalet-cache.json", m_inputs.buildPath()));
	m_removeOldCacheFolder = m_localConfig.json.empty();
}

/*****************************************************************************/
const std::string& WorkspaceCache::getCacheRef(const Type inCacheType) const
{
	return inCacheType == Type::Global ? m_cacheGlobal : m_cacheLocal;
}

/*****************************************************************************/
void WorkspaceCache::initialize(const std::string& inAppPath)
{
	const auto userDir = Environment::getUserDirectory();
	m_cacheGlobal = fmt::format("{}/.chalet", userDir);

	const auto& buildPath = m_inputs.buildPath();
	m_cacheLocal = fmt::format("{}/.cache", buildPath);

	makeAppVersionCheck(inAppPath);
}

/*****************************************************************************/
bool WorkspaceCache::createCacheFolder(const Type inCacheType)
{
	const auto& cacheRef = getCacheRef(inCacheType);

	if (m_removeOldCacheFolder)
	{
		removeCacheFolder(inCacheType);
		m_removeOldCacheFolder = false;
	}

	Output::setShowCommandOverride(false);

	if (!Commands::pathExists(cacheRef))
		return Commands::makeDirectory(cacheRef);

	Output::setShowCommandOverride(true);

	return true;
}

/*****************************************************************************/
bool WorkspaceCache::exists(const Type inCacheType) const
{
	const auto& cacheRef = getCacheRef(inCacheType);
	return Commands::pathExists(cacheRef) || Commands::pathExists(m_localConfig.filename());
}

/*****************************************************************************/
void WorkspaceCache::removeCacheFolder(const Type inCacheType)
{
	const auto& cacheRef = getCacheRef(inCacheType);

	if (Commands::pathExists(cacheRef))
		Commands::removeRecursively(cacheRef);
}

/*****************************************************************************/
std::string WorkspaceCache::getHash(const std::size_t inWorkspaceHash, const std::string& inIdentifier, const Type inCacheType) const
{
	std::string toHash = fmt::format("{}_{}", inWorkspaceHash, inIdentifier);
	std::string hash = Hash::string(toHash);

	const auto& cacheRef = getCacheRef(inCacheType);
	std::string ret = fmt::format("{}/{}", cacheRef, hash);

	// LOG(ret);

	return ret;
}

/*****************************************************************************/
std::string WorkspaceCache::getPath(const std::string& inFolder, const Type inCacheType) const
{
	const auto& cacheRef = getCacheRef(inCacheType);
	std::string ret;

	if (inFolder.empty())
		ret = cacheRef;
	else
		ret = fmt::format("{}/{}", cacheRef, inFolder);

	// LOG(ret);

	return ret;
}

/*****************************************************************************/
std::string WorkspaceCache::getCacheKey(const std::string& inName, const std::string& inConfig)
{
	return fmt::format("{}:{}", inConfig, inName);
}

/*****************************************************************************/
JsonFile& WorkspaceCache::localConfig() noexcept
{
	return m_localConfig;
}

/*****************************************************************************/
void WorkspaceCache::saveLocalConfig()
{
	m_localConfig.save();
}

/*****************************************************************************/
bool WorkspaceCache::appBuildChanged() const noexcept
{
	return m_appBuildChanged;
}

/*****************************************************************************/
bool WorkspaceCache::removeUnusedProjectFiles(const StringList& inHashes, const Type inCacheType)
{
	const auto& cacheRef = getCacheRef(inCacheType);
	if (!Commands::pathExists(cacheRef) || inHashes.size() == 0)
		return true;

	bool result = true;

	bool settingChanged = m_compileStrategyChanged;

	auto dirEnd = fs::directory_iterator();
	for (auto it = fs::directory_iterator(cacheRef); it != dirEnd; ++it)
	{
		auto item = *it;
		if (item.is_directory())
		{
			const auto& path = item.path();
			const auto stem = path.stem().string();

			if (!List::contains(inHashes, stem) || settingChanged)
				result &= Commands::removeRecursively(path.string());
		}
		else if (item.is_regular_file())
		{
			const auto& path = item.path();
			const auto filename = path.filename().string();

			if (!List::contains(inHashes, filename) || settingChanged)
				result &= Commands::remove(path.string());
		}
	}

	return result;
}

/*****************************************************************************/
void WorkspaceCache::removeStaleProjectCaches(const std::string& inToolchain, const Type inCacheType)
{
	UNUSED(inToolchain);
	// const auto& cacheRef = getCacheRef(inCacheType);
	// if (!m_localConfig.json.contains(kKeyData) || !m_localConfig.json.contains(kKeySettings))
	// 	return;

	// Json& buildCache = m_localConfig.json.at(kKeyData);
	// if (!buildCache.is_object())
	// 	return;

	// auto& toolchains = m_localConfig.json.at("toolchains");
	// if (!toolchains.is_object())
	// 	return;

	// auto& toolchainJson = toolchains[inToolchain];
	// if (!toolchainJson.is_object())
	// 	return;

	// auto& strategyJson = toolchainJson[kKeyStrategy];
	// if (!strategyJson.is_string())
	// 	return;

	// const auto strategy = strategyJson.get<std::string>();

	StringList hashes;
	/*
	for (auto it = buildCache.begin(); it != buildCache.end();)
	{
		const auto& key = it.key();
		const auto& splitKey = String::split(key, ':');
		const auto& keyBuildConfig = splitKey.front();

		const auto& name = splitKey.back();

		const bool validForBuild = splitKey.size() > 1 && (strategy == name);

		const bool internalKey = key == kKeyDataVersion
			|| key == kKeyDataVersionDebug
			|| key == kKeyDataStrategy
			|| key == kKeyDataTargetArchitecture
			|| key == kKeyDataWorkingDirectory;

		if (internalKey)
		{
			++it;
			continue;
		}

		if (it.value().is_string())
		{
			std::string hash = it.value().get<std::string>();
			hashes.push_back(std::move(hash));
			hashes.push_back(keyBuildConfig);

			std::string path = fmt::format("{}/{}", cacheRef, hash);
			if (key != kKeyDataSourceList)
			{
				if (!validForBuild && Commands::pathExists(path))
					Commands::removeRecursively(path);
			}
		}

		if (!validForBuild)
		{
			it = buildCache.erase(it);
			m_localConfig.setDirty(true);
		}
		else
		{
			++it;
		}
	}
	*/

	UNUSED(removeUnusedProjectFiles(hashes, inCacheType));
}

/*****************************************************************************/
void WorkspaceCache::removeBuildIfCacheChanged(const std::string& inBuildDir)
{
	if (!Commands::pathExists(inBuildDir))
		return;

	if (m_compileStrategyChanged || m_workingDirectoryChanged)
		Commands::removeRecursively(inBuildDir);
}

/*****************************************************************************/
void WorkspaceCache::makeAppVersionCheck(const std::string& inAppPath)
{
#if defined(CHALET_DEBUG)
	const auto& kKeyVer = kKeyDataVersionDebug;
	UNUSED(kKeyDataVersion);
#else
	const auto& kKeyVer = kKeyDataVersion;
	UNUSED(kKeyDataVersionDebug);
#endif

	if (!m_localConfig.json.contains(kKeyData))
		return;

	Json& data = m_localConfig.json[kKeyData];

	const auto buildHash = getBuildHash(inAppPath);
	std::string lastBuildHash;
	if (data.contains(kKeyVer))
	{
		lastBuildHash = data.at(kKeyVer).get<std::string>();
	}

	if (buildHash == lastBuildHash)
		return;

	data[kKeyVer] = buildHash;
	m_localConfig.setDirty(true);
	m_appBuildChanged = true;
}

/*****************************************************************************/
void WorkspaceCache::checkIfCompileStrategyChanged(const std::string& inToolchain)
{
	m_compileStrategyChanged = false;

	if (!m_localConfig.json.contains(kKeyData))
		return;

	Json& data = m_localConfig.json[kKeyData];
	if (!data.is_object())
		return;

	auto& toolchains = m_localConfig.json.at("toolchains");
	if (!toolchains.is_object())
		return;

	auto& toolchainJson = toolchains[inToolchain];
	if (!toolchainJson.is_object())
		return;

	auto& strategyJson = toolchainJson[kKeyStrategy];
	if (!strategyJson.is_string())
		return;

	const auto strategy = strategyJson.get<std::string>();

	const auto hashStrategy = Hash::string(strategy);

	if (!data.contains(kKeyDataStrategy))
	{
		data[kKeyDataStrategy] = hashStrategy;
		m_localConfig.setDirty(true);
		return;
	}
	else
	{
		const auto dataStrategy = data[kKeyDataStrategy].get<std::string>();
		if (dataStrategy != hashStrategy)
		{
			data[kKeyDataStrategy] = hashStrategy;
			m_localConfig.setDirty(true);
			// m_compileStrategyChanged = true;
		}
	}
}

/*****************************************************************************/
void WorkspaceCache::addSourceCache(const std::string& inHash)
{
	if (!m_localConfig.json.contains(kKeyData))
		return;

	Json& data = m_localConfig.json[kKeyData];
	if (!data.is_object())
		return;

	if (!data.contains(kKeyDataSourceList))
	{
		data[kKeyDataSourceList] = inHash;
		m_localConfig.setDirty(true);
	}
}

/*****************************************************************************/
void WorkspaceCache::checkIfWorkingDirectoryChanged()
{
	m_workingDirectoryChanged = false;

	if (!m_localConfig.json.contains(kKeyWorkingDirectory))
		return;

	Json& workingDirJson = m_localConfig.json[kKeyWorkingDirectory];

	if (!workingDirJson.is_string())
		return;

	const auto workingDirectory = workingDirJson.get<std::string>();

	if (!m_localConfig.json.contains(kKeyData))
		return;

	Json& data = m_localConfig.json[kKeyData];
	if (!data.is_object())
		return;

	const auto hashWorkingDir = Hash::string(workingDirectory);

	if (!data.contains(kKeyDataWorkingDirectory))
	{
		data[kKeyDataWorkingDirectory] = hashWorkingDir;
		m_localConfig.setDirty(true);
		return;
	}
	else
	{
		const auto dataWorkingDir = data[kKeyDataWorkingDirectory].get<std::string>();
		if (dataWorkingDir != hashWorkingDir)
		{
			data[kKeyDataWorkingDirectory] = hashWorkingDir;
			m_localConfig.setDirty(true);
			m_workingDirectoryChanged = true;
		}
	}
}

/*****************************************************************************/
std::string WorkspaceCache::getBuildHash(std::string appPath)
{
	Output::setShowCommandOverride(false);
	if (!Commands::pathExists(appPath))
	{
		appPath = Commands::which(appPath);
	}
	auto lastWrite = Commands::getLastWriteTime(appPath);
	Output::setShowCommandOverride(true);

	return Hash::string(std::to_string(lastWrite));
}
}
