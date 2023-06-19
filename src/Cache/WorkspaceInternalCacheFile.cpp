/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Cache/WorkspaceInternalCacheFile.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Terminal/Commands.hpp"
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
void WorkspaceInternalCacheFile::setBuildHash(const std::string& inValue, const bool inTemp) noexcept
{
	if (inTemp)
	{
		m_tempBuildHash = inValue;
	}
	else
	{
		m_buildHashChanged = m_buildHash != inValue;
		m_dirty |= m_buildHashChanged;
		m_buildHash = inValue;
		m_tempBuildHash.clear();
	}
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::sourceCacheAvailable() const
{
	return m_sources != nullptr;
}

/*****************************************************************************/
SourceCache& WorkspaceInternalCacheFile::sources() const
{
	chalet_assert(m_sources != nullptr, "");
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
		chalet_assert(m_initializedTime != 0, "");
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
						if (int val; m_dataFile->assignFromKey(val, value, CacheKeys::BuildLastBuildStrategy))
							m_sources->setLastBuildStrategy(val);

						if (value.contains(CacheKeys::DataCache))
						{
							auto& dataJson = value.at(CacheKeys::DataCache);
							if (dataJson.is_object())
							{
								for (auto& [file, data] : dataJson.items())
								{
									if (data.is_object())
									{
										for (auto& [key, val] : data.items())
										{
											auto rawValue = val.get<std::string>();
											m_sources->addDataCache(file, key.data(), rawValue);

											if (key == CacheKeys::BuildFiles)
											{
												std::time_t lastWrite = strtoll(rawValue.c_str(), NULL, 0);
												m_sources->addLastWrite(file, lastWrite);
											}
										}
									}
								}
							}
						}

						if (value.contains(CacheKeys::BuildFiles))
						{
							auto& files = value.at(CacheKeys::BuildFiles);
							if (files.is_object())
							{
								for (auto& [file, val] : files.items())
								{
									auto rawValue = val.get<std::string>();
									std::time_t lastWrite = strtoll(rawValue.c_str(), NULL, 0);
									m_sources->addLastWrite(file, lastWrite);
								}
							}
						}
					}
				}
			}
		}

		if (m_sources == nullptr)
		{
			auto [it, success] = m_sourceCaches.emplace(inId, std::make_unique<SourceCache>(m_initializedTime));
			if (!success)
			{
				Diagnostic::error("Error creating cache for {}", inId);
				return false;
			}

			m_sources = it->second.get();
		}
	}

	if (inStrategy != StrategyType::None)
		m_sources->setLastBuildStrategy(inStrategy, true);

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
					int lastStrategy = 0;
					if (build.contains(CacheKeys::BuildLastBuildStrategy))
					{
						auto& strat = build.at(CacheKeys::BuildLastBuildStrategy);
						if (strat.is_number())
							lastStrategy = strat.get<int>();
					}

					removeId = !build.is_object() || lastStrategy == static_cast<int>(StrategyType::Native);
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
bool WorkspaceInternalCacheFile::initialize(const std::string& inFilename, const std::string& inBuildFile)
{
	m_filename = inFilename;
	m_initializedTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	m_lastBuildFileWrite = Commands::getLastWriteTime(inBuildFile);

	chalet_assert(m_initializedTime != 0, "");

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
				m_buildHash = std::move(val);

			if (std::string val; m_dataFile->assignFromKey(val, hashes, CacheKeys::HashTheme))
				m_hashTheme = std::move(val);

			if (std::string val; m_dataFile->assignFromKey(val, hashes, CacheKeys::HashVersionRelease))
				m_hashVersion = std::move(val);

			if (std::string val; m_dataFile->assignFromKey(val, hashes, CacheKeys::HashMetadata))
				m_hashMetadata = std::move(val);

			if (std::string val; m_dataFile->assignFromKey(val, hashes, CacheKeys::HashVersionDebug))
				m_hashVersionDebug = std::move(val);

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

	// configure step without building first would not have one. don't do anything
	auto& buildHash = !m_tempBuildHash.empty() ? m_tempBuildHash : m_buildHash;
	if (buildHash.empty())
		return true;

	for (auto& [_, sourceCache] : m_sourceCaches)
	{
		m_dirty |= sourceCache->dirty();
	}

	if (m_dirty && m_dataFile != nullptr && !m_disallowSave)
	{
		Json rootNode = Json::object();

		rootNode[CacheKeys::Hashes] = Json::object();

		if (!buildHash.empty())
			rootNode[CacheKeys::Hashes][CacheKeys::HashBuild] = buildHash;

		if (!m_hashTheme.empty())
			rootNode[CacheKeys::Hashes][CacheKeys::HashTheme] = m_hashTheme;

		if (!m_hashVersionDebug.empty())
			rootNode[CacheKeys::Hashes][CacheKeys::HashVersionDebug] = m_hashVersionDebug;

		if (!m_hashVersion.empty())
			rootNode[CacheKeys::Hashes][CacheKeys::HashVersionRelease] = m_hashVersion;

		if (!m_hashMetadata.empty())
			rootNode[CacheKeys::Hashes][CacheKeys::HashMetadata] = m_hashMetadata;

		rootNode[CacheKeys::Hashes][CacheKeys::HashExtra] = Json::array();
		for (auto& hash : m_extraHashes)
		{
			rootNode[CacheKeys::Hashes][CacheKeys::HashExtra].push_back(hash);
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
bool WorkspaceInternalCacheFile::forceRebuild() const noexcept
{
	return m_forceRebuild;
}

void WorkspaceInternalCacheFile::setForceRebuild(const bool inValue)
{
	m_forceRebuild = inValue;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::buildStrategyChanged() const
{
	return m_forceRebuild || sources().buildStrategyChanged();
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
void WorkspaceInternalCacheFile::setDisallowSave(const bool inValue)
{
	m_disallowSave = inValue;
}

/*****************************************************************************/
std::string WorkspaceInternalCacheFile::getAppVersionHash(const std::string& inAppPath)
{
	Output::setShowCommandOverride(false);
	auto lastWrite = Commands::getLastWriteTime(inAppPath);
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
