/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildCache.hpp"

#include "BuildJson/BuildEnvironment.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
BuildCache::BuildCache(const WorkspaceInfo& inInfo, const BuildPaths& inPaths) :
	m_info(inInfo),
	m_paths(inPaths)
{
	m_environmentCache.load(fmt::format("{}/chalet-cache.json", m_paths.buildDir()));
	m_removeOldCacheFolder = m_environmentCache.json.empty();
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

	const auto& buildDir = m_paths.buildDir();
	m_cacheLocal = fmt::format("{}/.cache", buildDir);

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
void BuildCache::removeCacheFolder(const Type inCacheType)
{
	const auto& cacheRef = getCacheRef(inCacheType);

	if (Commands::pathExists(cacheRef))
		Commands::removeRecursively(cacheRef);
}

/*****************************************************************************/
std::string BuildCache::getHash(const std::string& inIdentifier, const Type inCacheType) const
{
	bool isBash = Environment::isBash();
	std::size_t infoHash = m_info.hash();

	std::string toHash = fmt::format("{}_{}_{}", infoHash, inIdentifier, isBash ? "bash" : "cmd");
	std::string hash = Hash::string(toHash);

	const auto& cacheRef = getCacheRef(inCacheType);

	std::string ret = fmt::format("{}/{}", cacheRef, hash);

	// LOG(ret);

	return ret;
}

/*****************************************************************************/
JsonFile& BuildCache::environmentCache() noexcept
{
	return m_environmentCache;
}

/*****************************************************************************/
void BuildCache::saveEnvironmentCache()
{
	if (m_dirty)
		m_environmentCache.save();
}

/*****************************************************************************/
bool BuildCache::dirty() const noexcept
{
	return m_dirty;
}
void BuildCache::setDirty(const bool inValue) noexcept
{
	m_dirty = inValue;
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

	auto dirEnd = fs::directory_iterator();
	for (auto it = fs::directory_iterator(cacheRef); it != dirEnd; ++it)
	{
		auto item = *it;
		if (item.is_directory())
		{
			const auto& path = item.path();
			const auto stem = path.stem().string();

			if (!List::contains(inHashes, stem) || m_compileStrategyChanged)
				result &= Commands::removeRecursively(path.string());
		}
		else if (item.is_regular_file())
		{
			const auto& path = item.path();
			const auto filename = path.filename().string();

			if (!List::contains(inHashes, filename) || m_compileStrategyChanged)
				result &= Commands::remove(path.string());
		}
	}

	return result;
}

/*****************************************************************************/
void BuildCache::removeStaleProjectCaches(const std::string& inBuildConfig, const StringList& inProjectNames, const Type inCacheType)
{
	const auto& cacheRef = getCacheRef(inCacheType);
	if (!m_environmentCache.json.contains(kKeyData))
		return;

	Json& buildCache = m_environmentCache.json.at(kKeyData);
	if (buildCache.type() != JsonDataType::object)
		return;

	StringList hashes;
	for (auto it = buildCache.begin(); it != buildCache.end();)
	{
		const auto& key = it.key();
		const auto& splitKey = String::split(key, ":");
		const auto& keyBuildConfig = splitKey.front();

		const auto& name = splitKey.back();

		const bool validForBuild = splitKey.size() > 1 && (inBuildConfig == keyBuildConfig || List::contains<std::string>(inProjectNames, name));

		const bool internalKey = key == kKeyDataVersion || key == kKeyDataVersionDebug || key == kKeyDataStrategy || key == kKeyDataWorkingDirectory;
		if (internalKey)
		{
			++it;
			continue;
		}

		if (it.value().is_string())
		{
			std::string hash = it.value().get<std::string>();
			hashes.push_back(std::move(hash));

			std::string path = fmt::format("{}/{}", cacheRef, hash);
			if (!validForBuild && Commands::pathExists(path))
				Commands::removeRecursively(path);
		}

		if (!validForBuild)
		{
			it = buildCache.erase(it);
			setDirty(true);
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

	if (!m_environmentCache.json.contains(kKeyData))
		return;

	Json& data = m_environmentCache.json[kKeyData];

	const auto buildHash = getBuildHash(inAppPath);
	std::string lastBuildHash;
	if (data.contains(kKeyVer))
	{
		lastBuildHash = data.at(kKeyVer).get<std::string>();
	}

	if (buildHash == lastBuildHash)
		return;

	data[kKeyVer] = buildHash;
	setDirty(true);
	m_appBuildChanged = true;
}

/*****************************************************************************/
void BuildCache::checkIfCompileStrategyChanged()
{
	m_compileStrategyChanged = false;

	if (!m_environmentCache.json.contains(kKeyStrategy))
		return;

	Json& strategyJson = m_environmentCache.json[kKeyStrategy];

	if (!strategyJson.is_string())
		return;

	const auto strategy = strategyJson.get<std::string>();

	if (!m_environmentCache.json.contains(kKeyData))
		return;

	Json& data = m_environmentCache.json[kKeyData];
	if (!data.is_object())
		return;

	const auto hashStrategy = Hash::string(strategy);

	if (!data.contains(kKeyDataStrategy))
	{
		data[kKeyDataStrategy] = hashStrategy;
		setDirty(true);
		return;
	}
	else
	{
		const auto dataStrategy = data[kKeyDataStrategy].get<std::string>();
		if (dataStrategy != hashStrategy)
		{
			data[kKeyDataStrategy] = hashStrategy;
			setDirty(true);
			m_compileStrategyChanged = true;
		}
	}
}

/*****************************************************************************/
void BuildCache::checkIfWorkingDirectoryChanged()
{
	m_workingDirectoryChanged = false;

	if (!m_environmentCache.json.contains(kKeyWorkingDirectory))
		return;

	Json& workingDirJson = m_environmentCache.json[kKeyWorkingDirectory];

	if (!workingDirJson.is_string())
		return;

	const auto workingDirectory = workingDirJson.get<std::string>();

	if (!m_environmentCache.json.contains(kKeyData))
		return;

	Json& data = m_environmentCache.json[kKeyData];
	if (!data.is_object())
		return;

	const auto hashWorkingDir = Hash::string(workingDirectory);

	if (!data.contains(kKeyDataWorkingDirectory))
	{
		data[kKeyDataWorkingDirectory] = hashWorkingDir;
		setDirty(true);
		return;
	}
	else
	{
		const auto dataWorkingDir = data[kKeyDataWorkingDirectory].get<std::string>();
		if (dataWorkingDir != hashWorkingDir)
		{
			data[kKeyDataWorkingDirectory] = hashWorkingDir;
			setDirty(true);
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
	if (Environment::isBash())
	{
		const std::string md5Result = Commands::subprocessOutput({ "md5sum", appPath });
		auto list = String::split(md5Result, " ");

		md5 = list.front();
		String::replaceAll(md5, "\\", "");
	}
	else
	{
		const std::string md5Result = Commands::subprocessOutput({ "certutil", "-hashfile", appPath, "MD5" });
	#ifdef CHALET_MSVC
		std::string_view eol = "\r\n";
	#else
		std::string_view eol = "\n";
	#endif
		auto list = String::split(md5Result, eol);
		if (list.size() >= 2)
		{
			md5 = list[1];
		}
	}
#elif defined(CHALET_MACOS)
	std::string md5Result = Commands::subprocessOutput({ "md5", appPath });
	auto list = String::split(md5Result, " ");

	md5 = list.back();
#else
	const std::string md5Result = Commands::subprocessOutput({ "md5sum", appPath });
	auto list = String::split(md5Result, " ");

	md5 = list.front();
	String::replaceAll(md5, "\\", "");
#endif

	// LOG(md5);

	return Hash::string(md5);
}
}
