/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildCache.hpp"

#include "Libraries/Format.hpp"
#include "State/BuildEnvironment.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
BuildCache::BuildCache(const CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
	m_localConfig.load(fmt::format("{}/chalet-cache.json", m_inputs.buildPath()));
	m_removeOldCacheFolder = m_localConfig.json.empty();
}

/*****************************************************************************/
const std::string& BuildCache::getCacheRef(const Type inCacheType) const
{
	return inCacheType == Type::Global ? m_cacheGlobal : m_cacheLocal;
}

/*****************************************************************************/
void BuildCache::initialize(const std::string& inAppPath)
{
	const auto userDir = Environment::getUserDirectory();
	m_cacheGlobal = fmt::format("{}/.chalet", userDir);

	const auto& buildPath = m_inputs.buildPath();
	m_cacheLocal = fmt::format("{}/.cache", buildPath);

	makeAppVersionCheck(inAppPath);
}

/*****************************************************************************/
bool BuildCache::createCacheFolder(const Type inCacheType)
{
	const auto& cacheRef = getCacheRef(inCacheType);

	if (m_removeOldCacheFolder)
	{
		removeCacheFolder(inCacheType);
		m_removeOldCacheFolder = false;
	}

	try
	{
		if (!Commands::pathExists(cacheRef))
			fs::create_directory(cacheRef);

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
}

/*****************************************************************************/
bool BuildCache::exists(const Type inCacheType) const
{
	const auto& cacheRef = getCacheRef(inCacheType);
	return Commands::pathExists(cacheRef) || Commands::pathExists(m_localConfig.filename());
}

/*****************************************************************************/
void BuildCache::removeCacheFolder(const Type inCacheType)
{
	const auto& cacheRef = getCacheRef(inCacheType);

	if (Commands::pathExists(cacheRef))
		Commands::removeRecursively(cacheRef);
}

/*****************************************************************************/
std::string BuildCache::getHash(const std::size_t inWorkspaceHash, const std::string& inIdentifier, const Type inCacheType) const
{
	std::string toHash = fmt::format("{}_{}", inWorkspaceHash, inIdentifier);
	std::string hash = Hash::string(toHash);

	const auto& cacheRef = getCacheRef(inCacheType);
	std::string ret = fmt::format("{}/{}", cacheRef, hash);

	// LOG(ret);

	return ret;
}

/*****************************************************************************/
std::string BuildCache::getPath(const std::string& inFolder, const Type inCacheType) const
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
std::string BuildCache::getCacheKey(const std::string& inName, const std::string& inConfig)
{
	return fmt::format("{}:{}", inConfig, inName);
}

/*****************************************************************************/
JsonFile& BuildCache::localConfig() noexcept
{
	return m_localConfig;
}

/*****************************************************************************/
void BuildCache::saveLocalConfig()
{
	m_localConfig.save();
}

/*****************************************************************************/
bool BuildCache::appBuildChanged() const noexcept
{
	return m_appBuildChanged;
}

/*****************************************************************************/
bool BuildCache::removeUnusedProjectFiles(const StringList& inHashes, const Type inCacheType)
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
void BuildCache::removeStaleProjectCaches(const std::string& inToolchain, const Type inCacheType)
{
	const auto& cacheRef = getCacheRef(inCacheType);
	if (!m_localConfig.json.contains(kKeyData) || !m_localConfig.json.contains(kKeySettings))
		return;

	Json& buildCache = m_localConfig.json.at(kKeyData);
	if (!buildCache.is_object())
		return;

	auto& compilerTools = m_localConfig.json.at("compilerTools");
	if (!compilerTools.is_object())
		return;

	auto& toolchainJson = compilerTools[inToolchain];
	if (!toolchainJson.is_object())
		return;

	auto& strategyJson = toolchainJson[kKeyStrategy];
	if (!strategyJson.is_string())
		return;

	const auto strategy = strategyJson.get<std::string>();

	StringList hashes;
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

	UNUSED(removeUnusedProjectFiles(hashes, inCacheType));
}

/*****************************************************************************/
void BuildCache::removeBuildIfCacheChanged(const std::string& inBuildDir)
{
	if (!Commands::pathExists(inBuildDir))
		return;

	if (m_compileStrategyChanged || m_workingDirectoryChanged)
		Commands::removeRecursively(inBuildDir);
}

/*****************************************************************************/
void BuildCache::makeAppVersionCheck(const std::string& inAppPath)
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
void BuildCache::checkIfCompileStrategyChanged(const std::string& inToolchain)
{
	m_compileStrategyChanged = false;

	if (!m_localConfig.json.contains(kKeyData))
		return;

	Json& data = m_localConfig.json[kKeyData];
	if (!data.is_object())
		return;

	auto& compilerTools = m_localConfig.json.at("compilerTools");
	if (!compilerTools.is_object())
		return;

	auto& toolchainJson = compilerTools[inToolchain];
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
			m_compileStrategyChanged = true;
		}
	}
}

/*****************************************************************************/
void BuildCache::addSourceCache(const std::string& inHash)
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
void BuildCache::checkIfWorkingDirectoryChanged()
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
std::string BuildCache::getBuildHash(std::string appPath)
{
	// TODO: Keep an eye on this...

	if (!Commands::pathExists(appPath))
	{
		// Get the exact path via which
		// Note: There's no fs equivalent of this (fs::absolute returns a garbage/non-existent path)
		appPath = Commands::which(appPath);
	}

	std::string md5;
#if defined(CHALET_WIN32)
	/*if (Environment::isBash())
	{
		const std::string md5Result = Commands::subprocessOutput({ "md5sum", appPath });
		auto list = String::split(md5Result);

		md5 = list.front();
		String::replaceAll(md5, '\\', 0);
	}
	else*/
	{
		const std::string md5Result = Commands::subprocessOutput({ "cmd.exe", "/c", "certutil", "-hashfile", appPath, "MD5" });
		auto list = String::split(md5Result, String::eol());
		if (list.size() >= 2)
		{
			md5 = list[1];
		}
	}
#elif defined(CHALET_MACOS)
	std::string md5Result = Commands::subprocessOutput({ "md5", appPath });
	auto list = String::split(md5Result);

	md5 = list.back();
#else
	const std::string md5Result = Commands::subprocessOutput({ "md5sum", appPath });
	auto list = String::split(md5Result);

	md5 = list.front();
	String::replaceAll(md5, '\\', 0);
#endif

	// LOG(md5);

	return Hash::string(md5);
}
}
