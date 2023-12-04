/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Cache/WorkspaceInternalCacheFile.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
WorkspaceInternalCacheFile::WorkspaceInternalCacheFile() = default;

/*****************************************************************************/
WorkspaceInternalCacheFile::~WorkspaceInternalCacheFile() = default;

/*****************************************************************************/
bool WorkspaceInternalCacheFile::sourceCacheAvailable() const
{
	return m_sources != nullptr;
}

/*****************************************************************************/
SourceCache& WorkspaceInternalCacheFile::sources() const
{
	chalet_assert(m_sources != nullptr, "source cache was not set");
	return *m_sources;
}

/*****************************************************************************/
ExternalDependencyCache& WorkspaceInternalCacheFile::externalDependencies()
{
	return m_externalDependencies;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::setSourceCache(const std::string& inId, const StrategyType inStrategy)
{
	auto itr = m_sourceCaches.find(inId);
	if (itr != m_sourceCaches.end())
	{
		m_sources = itr->second.get();
	}
	else
	{
		chalet_assert(m_dataFile != nullptr, "");

		m_sources = nullptr;

		auto& rootNode = m_dataFile->json;
		if (rootNode.contains(CacheKeys::Builds))
		{
			auto& builds = rootNode.at(CacheKeys::Builds);
			if (builds.is_object() && builds.contains(inId))
			{
				auto& value = builds.at(inId);
				if (value.is_object())
				{
					if (std::string rawValue; m_dataFile->assignFromKey(rawValue, value, CacheKeys::BuildLastBuilt))
					{
						std::time_t lastBuild = strtoll(rawValue.c_str(), NULL, 0);
						auto [it, success] = m_sourceCaches.emplace(inId, std::make_unique<SourceCache>(lastBuild));
						if (!success)
						{
							Diagnostic::error("Error creating cache for {}", inId);
							return false;
						}

						m_sources = it->second.get();
					}

					if (m_sources != nullptr)
					{
						if (i32 val; m_dataFile->assignFromKey(val, value, CacheKeys::BuildLastBuildStrategy))
							m_sources->setLastBuildStrategy(val);

						if (value.contains(CacheKeys::HashDataCache))
						{
							auto& dataJson = value.at(CacheKeys::HashDataCache);
							if (dataJson.is_object())
							{
								for (auto& [key, val] : dataJson.items())
								{
									if (val.is_string())
									{
										m_sources->addDataCache(key, val.get<std::string>());
									}
								}
							}
						}

						if (value.contains(CacheKeys::BuildFiles))
						{
							auto& files = value.at(CacheKeys::BuildFiles);
							if (files.is_array())
							{
								for (auto& hash : files)
								{
									if (hash.is_number_unsigned())
									{
										m_sources->addToFileCache(hash.get<size_t>());
									}
								}
							}
						}
					}
				}
			}
		}

		if (m_sources == nullptr)
		{
			auto [it, success] = m_sourceCaches.emplace(inId, std::make_unique<SourceCache>(0));
			if (!success)
			{
				Diagnostic::error("Error creating cache for {}", inId);
				return false;
			}

			m_sources = it->second.get();
		}
	}

	if (inStrategy != StrategyType::None)
		m_sources->setLastBuildStrategy(static_cast<i32>(inStrategy), true);

	m_sources->updateInitializedTime();

	return true;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::removeSourceCache(const std::string& inId)
{
	bool result = false;
	auto& rootNode = m_dataFile->json;
	if (rootNode.contains(CacheKeys::Builds))
	{
		auto& builds = rootNode.at(CacheKeys::Builds);
		if (builds.is_object())
		{
			if (builds.contains(inId))
			{
				bool removeId = false;
				{
					auto& build = builds.at(inId);
					i32 lastStrategy = 0;
					if (build.contains(CacheKeys::BuildLastBuildStrategy))
					{
						auto& strat = build.at(CacheKeys::BuildLastBuildStrategy);
						if (strat.is_number())
							lastStrategy = strat.get<i32>();
					}

					removeId = !build.is_object() || lastStrategy == static_cast<i32>(StrategyType::Native);
				}
				if (removeId)
				{
					builds.erase(inId);
					m_dirty = true;
					result = true;
				}
			}
		}
	}

	auto itr = m_sourceCaches.find(inId);
	if (itr != m_sourceCaches.end())
	{
		if (itr->second->canRemoveCachedFolder())
		{
			if (m_sources == itr->second.get())
				m_sources = nullptr;

			itr = m_sourceCaches.erase(itr);
			m_dirty = true;
			result = true;
		}
	}

	return result;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::removeExtraCache(const std::string& inId)
{
	bool result = false;
	for (auto itr = m_extraHashes.begin(); itr != m_extraHashes.end();)
	{
		if (*itr == inId)
		{
			itr = m_extraHashes.erase(itr);
			m_dirty = true;
			result = true;
		}
		else
		{
			++itr;
		}
	}

	return result;
}

/*****************************************************************************/
void WorkspaceInternalCacheFile::setBuildHash(const std::string& inValue) noexcept
{
	m_buildHashChanged = m_hashBuild != inValue;
	m_dirty |= m_buildHashChanged;
	m_hashBuild = inValue;
}

/*****************************************************************************/
void WorkspaceInternalCacheFile::setBuildOutputCache(const std::string& inPath, const std::string& inToolchain)
{
	if (m_outputPathCache.find(inPath) != m_outputPathCache.end())
	{
		auto& toolchain = m_outputPathCache.at(inPath);
		m_toolchainChangedForBuildOutputPath = toolchain != inToolchain;
		if (m_toolchainChangedForBuildOutputPath)
		{
			m_outputPathCache[inPath] = inToolchain;
			m_dirty = true;
		}
	}
	else
	{
		m_outputPathCache.emplace(inPath, inToolchain);
		m_toolchainChangedForBuildOutputPath = false;
		m_dirty = true;
	}
}

/*****************************************************************************/
std::string WorkspaceInternalCacheFile::getDataValue(const std::string& inHash, const GetDataCallback& onGet)
{
	auto result = getDataCacheValue(inHash);
	if (result.empty())
	{
		if (onGet != nullptr)
			result = onGet();

		addToDataCache(inHash, result);
	}
	return result;
}

/*****************************************************************************/
std::string WorkspaceInternalCacheFile::getDataValueFromPath(const std::string& inPath, const GetDataCallback& onGet)
{
	chalet_assert(m_sources != nullptr, "getDataValueFromPath called before sources were set");

	auto hash = Hash::string(inPath);
	auto result = getDataCacheValue(hash);
	if (result.empty() || m_sources->fileChangedOrDoesNotExist(inPath))
	{
		if (onGet != nullptr)
			result = onGet();

		addToDataCache(hash, result);
	}

	return result;
}

/*****************************************************************************/
void WorkspaceInternalCacheFile::addToDataCache(const std::string& inKey, std::string value)
{
	String::replaceAll(value, String::eol(), "");
#if defined(CHALET_WIN32)
	String::replaceAll(value, "\n", "");
#endif

	if (m_dataCache.find(inKey) != m_dataCache.end())
	{
		m_dataCache[inKey] = value;
	}
	else
	{
		m_dataCache.emplace(inKey, std::move(value));
	}
}

/*****************************************************************************/
const std::string& WorkspaceInternalCacheFile::getDataCacheValue(const std::string& inKey)
{
	if (m_dataCache.find(inKey) == m_dataCache.end())
	{
		m_dataCache[inKey] = std::string();
	}

	return m_dataCache.at(inKey);
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::initialize(const std::string& inFilename, const std::string& inBuildFile)
{
	m_filename = inFilename;
	m_lastBuildFileWrite = Files::getLastWriteTime(inBuildFile);

	m_dataFile = std::make_unique<JsonFile>(m_filename);
	if (!m_dataFile->load(false))
	{
		Diagnostic::clearErrors();
		m_dataFile->json = Json::object();
	}

	if (!m_dataFile->json.is_object())
		m_dataFile->json = Json::object();

	auto& rootNode = m_dataFile->json;
	if (rootNode.contains(CacheKeys::Hashes))
	{
		auto& hashes = rootNode.at(CacheKeys::Hashes);
		if (hashes.is_object())
		{
			if (std::string val; m_dataFile->assignFromKey(val, hashes, CacheKeys::HashBuild))
				m_hashBuild = std::move(val);

			if (std::string val; m_dataFile->assignFromKey(val, hashes, CacheKeys::HashTheme))
				m_hashTheme = std::move(val);

			if (std::string val; m_dataFile->assignFromKey(val, hashes, CacheKeys::HashVersionRelease))
				m_hashVersion = std::move(val);

			if (std::string val; m_dataFile->assignFromKey(val, hashes, CacheKeys::HashMetadata))
				m_hashMetadata = std::move(val);

			if (std::string val; m_dataFile->assignFromKey(val, hashes, CacheKeys::HashVersionDebug))
				m_hashVersionDebug = std::move(val);

			if (hashes.contains(CacheKeys::HashPathCache))
			{
				auto& pathCache = hashes.at(CacheKeys::HashPathCache);
				if (pathCache.is_object())
				{
					for (auto& [key, value] : pathCache.items())
					{
						m_outputPathCache.emplace(key, value);
					}
				}
			}

			if (hashes.contains(CacheKeys::HashDataCache))
			{
				auto& dataCache = hashes.at(CacheKeys::HashDataCache);
				if (dataCache.is_object())
				{
					for (auto& [key, value] : dataCache.items())
					{
						m_dataCache.emplace(key, value);
					}
				}
			}

			if (hashes.contains(CacheKeys::HashExtra))
			{
				auto& extra = hashes.at(CacheKeys::HashExtra);
				if (extra.is_array())
				{
					for (auto& item : extra)
					{
						if (!item.is_string())
							return false;

						std::string hash = item.get<std::string>();
						if (!hash.empty())
							addExtraHash(std::move(hash));
					}
				}
			}
		}
	}

	if (std::string rawValue; m_dataFile->assignFromKey(rawValue, rootNode, CacheKeys::LastChaletJsonWriteTime))
	{
		std::time_t val = strtoll(rawValue.c_str(), NULL, 0);
		m_buildFileChanged = val != m_lastBuildFileWrite;
	}

	return true;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::save()
{
	m_sources = nullptr;

	if (m_filename.empty())
		return false;

	if (m_hashBuild.empty())
		return true;

	for (auto& [_, sourceCache] : m_sourceCaches)
	{
		m_dirty |= sourceCache->dirty();
	}

	if (m_dirty && m_dataFile != nullptr)
	{
		Json rootNode = Json::object();

		rootNode[CacheKeys::Hashes] = Json::object();

		if (!m_hashBuild.empty())
			rootNode[CacheKeys::Hashes][CacheKeys::HashBuild] = m_hashBuild;

		if (!m_hashTheme.empty())
			rootNode[CacheKeys::Hashes][CacheKeys::HashTheme] = m_hashTheme;

		if (!m_hashMetadata.empty())
			rootNode[CacheKeys::Hashes][CacheKeys::HashMetadata] = m_hashMetadata;

		if (!m_hashVersionDebug.empty())
			rootNode[CacheKeys::Hashes][CacheKeys::HashVersionDebug] = m_hashVersionDebug;

		if (!m_hashVersion.empty())
			rootNode[CacheKeys::Hashes][CacheKeys::HashVersionRelease] = m_hashVersion;

		rootNode[CacheKeys::Hashes][CacheKeys::HashExtra] = Json::array();
		for (auto& hash : m_extraHashes)
		{
			rootNode[CacheKeys::Hashes][CacheKeys::HashExtra].push_back(hash);
		}

		rootNode[CacheKeys::Hashes][CacheKeys::HashPathCache] = Json::object();
		for (auto& [path, toolchain] : m_outputPathCache)
		{
			rootNode[CacheKeys::Hashes][CacheKeys::HashPathCache][path] = toolchain;
		}

		rootNode[CacheKeys::Hashes][CacheKeys::HashDataCache] = Json::object();
		for (auto& [key, value] : m_dataCache)
		{
			rootNode[CacheKeys::Hashes][CacheKeys::HashDataCache][key] = value;
		}

		rootNode[CacheKeys::LastChaletJsonWriteTime] = std::to_string(m_lastBuildFileWrite);

		if (!m_dataFile->json.contains(CacheKeys::Builds))
			rootNode[CacheKeys::Builds] = Json::object();
		else
			rootNode[CacheKeys::Builds] = m_dataFile->json.at(CacheKeys::Builds);

		for (auto& [id, sourceCache] : m_sourceCaches)
		{
			rootNode[CacheKeys::Builds][id] = sourceCache->asJson();
		}

		m_dataFile->setContents(std::move(rootNode));
		m_dataFile->setDirty(true);

		m_dataFile->save(-1);

		m_dirty = false;

		return true;
	}

	return true;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::loadExternalDependencies(const std::string& inPath)
{
	return m_externalDependencies.loadFromPath(inPath);
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::saveExternalDependencies()
{
	m_dirty |= m_externalDependencies.save();
	return true;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::buildHashChanged() const noexcept
{
	return m_buildHashChanged;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::buildFileChanged() const noexcept
{
	return m_buildFileChanged;
}

/*****************************************************************************/
void WorkspaceInternalCacheFile::setForceRebuild(const bool inValue)
{
	m_forceRebuild = inValue;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::canWipeBuildFolder() const
{
	return m_forceRebuild || m_toolchainChangedForBuildOutputPath || sources().buildStrategyChanged();
}

/*****************************************************************************/
void WorkspaceInternalCacheFile::checkForMetadataChange(const std::string& inHash)
{
	if (!m_metadataChanged.has_value())
	{
		m_metadataChanged = !String::equals(inHash, m_hashMetadata);
		m_hashMetadata = inHash;
	}
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::metadataChanged() const
{
	if (m_metadataChanged.has_value())
		return *m_metadataChanged;

	return false;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::themeChanged() const noexcept
{
	return m_themeChanged;
}

/*****************************************************************************/
void WorkspaceInternalCacheFile::checkIfThemeChanged()
{
	m_themeChanged = false;

	auto themeHash = Hash::string(Output::theme().asString());
	if (themeHash != m_hashTheme)
	{
		m_hashTheme = std::move(themeHash);
		m_themeChanged = true;
		m_dirty = true;
	}
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::appVersionChanged() const noexcept
{
	// return false;
	return m_appVersionChanged;
}

/*****************************************************************************/
void WorkspaceInternalCacheFile::checkIfAppVersionChanged(const std::string& inAppPath)
{
	m_appVersionChanged = false;

	auto versionHash = getAppVersionHash(inAppPath);
#if defined(CHALET_DEBUG)
	const auto& lastVersionHash = m_hashVersionDebug;
#else
	const auto& lastVersionHash = m_hashVersion;
#endif

	if (versionHash != lastVersionHash)
	{
#if defined(CHALET_DEBUG)
		m_hashVersionDebug = std::move(versionHash);
#else
		m_hashVersion = std::move(versionHash);
#endif
		m_appVersionChanged = true;
		m_dirty = true;
	}
}

/*****************************************************************************/
void WorkspaceInternalCacheFile::addExtraHash(std::string&& inHash)
{
	List::addIfDoesNotExist(m_extraHashes, std::move(inHash));
}

/*****************************************************************************/
std::string WorkspaceInternalCacheFile::getAppVersionHash(std::string appPath) const
{
	Output::setShowCommandOverride(false);
	if (!Files::pathExists(appPath))
	{
		appPath = Files::which(appPath);
	}

	auto lastWrite = Files::getLastWriteTime(appPath);
	Output::setShowCommandOverride(true);

	return Hash::string(std::to_string(lastWrite));
}

/*****************************************************************************/
StringList WorkspaceInternalCacheFile::getCacheIdsToNotRemove() const
{
	StringList ret;
	ret.emplace_back(String::getPathFilename(m_filename));

	for (auto& id : m_extraHashes)
	{
		List::addIfDoesNotExist(ret, id);
	}

	for (auto& [id, _] : m_sourceCaches)
	{
		List::addIfDoesNotExist(ret, id);
	}

	auto& rootNode = m_dataFile->json;
	if (rootNode.contains(CacheKeys::Builds))
	{
		auto& builds = rootNode.at(CacheKeys::Builds);
		if (builds.is_object())
		{
			for (auto [id, _] : builds.items())
			{
				List::addIfDoesNotExist(ret, id);
			}
		}
	}

	return ret;
}
}
