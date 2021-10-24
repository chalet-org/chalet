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

namespace chalet
{
/*****************************************************************************/
WorkspaceInternalCacheFile::WorkspaceInternalCacheFile() :
	kKeyHashes("h"),
	kKeyHashBuild("b"),
	kKeyHashTheme("t"),
	kKeyHashVersionDebug("vd"),
	kKeyHashVersionRelease("vr"),
	kKeyHashExtra("e"),
	kKeyLastChaletJsonWriteTime("c"),
	kKeyBuilds("d"),
	kKeyBuildLastBuilt("l"),
	kKeyBuildNative("n"),
	kKeyBuildFiles("f"),
	kKeyDataCache("x")
{
}

/*****************************************************************************/
WorkspaceInternalCacheFile::~WorkspaceInternalCacheFile() = default;

/*****************************************************************************/
void WorkspaceInternalCacheFile::setBuildHash(const std::string& inValue) noexcept
{
	m_buildHashChanged = m_buildHash != inValue;
	m_dirty |= m_buildHashChanged;
	m_buildHash = std::move(inValue);
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
bool WorkspaceInternalCacheFile::setSourceCache(const std::string& inId, const bool inNative)
{
	setBuildHash(inId);

	if (inNative)
		List::addIfDoesNotExist(m_doNotRemoves, inId);

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
		if (rootNode.contains(kKeyBuilds))
		{
			auto& builds = rootNode.at(kKeyBuilds);
			if (builds.is_object() && builds.contains(inId))
			{
				auto& value = builds.at(inId);
				if (value.is_object())
				{
					if (std::string rawValue; m_dataFile->assignFromKey(rawValue, value, kKeyBuildLastBuilt))
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
						if (bool val; m_dataFile->assignFromKey(val, value, kKeyBuildNative))
							m_sources->setNative(val);

						if (value.contains(kKeyDataCache))
						{
							auto& dataJson = value.at(kKeyDataCache);
							if (dataJson.is_object())
							{
								for (auto& [file, data] : dataJson.items())
								{
									if (data.is_object())
									{
										for (auto& [key, val] : data.items())
										{
											auto rawValue = val.get<std::string>();
											m_sources->addDataCache(file, key, rawValue);

											if (key == kKeyBuildFiles)
											{
												std::time_t lastWrite = strtoll(rawValue.c_str(), NULL, 0);
												m_sources->addLastWrite(file, lastWrite);
											}
										}
									}
								}
							}
						}

						if (value.contains(kKeyBuildFiles))
						{
							auto& files = value.at(kKeyBuildFiles);
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

	m_sources->setNative(inNative);
	m_sources->updateInitializedTime();

	return true;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::removeSourceCache(const std::string& inId)
{
	bool result = false;
	auto& rootNode = m_dataFile->json;
	if (rootNode.contains(kKeyBuilds))
	{
		auto& builds = rootNode.at(kKeyBuilds);
		if (builds.is_object())
		{
			if (builds.contains(inId))
			{
				bool removeId = false;
				{
					auto& build = builds.at(inId);
					removeId = !build.is_object() || !build.contains(kKeyBuildNative);
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
		if (!itr->second->native())
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
	if (!m_dataFile->load())
	{
		Diagnostic::clearErrors();
		m_dataFile->json = Json::object();
	}

	if (!m_dataFile->json.is_object())
		m_dataFile->json = Json::object();

	auto& rootNode = m_dataFile->json;
	if (rootNode.contains(kKeyHashes))
	{
		auto& hashes = rootNode.at(kKeyHashes);
		if (hashes.is_object())
		{
			if (std::string val; m_dataFile->assignFromKey(val, hashes, kKeyHashBuild))
				m_buildHash = std::move(val);

			if (std::string val; m_dataFile->assignFromKey(val, hashes, kKeyHashTheme))
				m_hashTheme = std::move(val);

			if (std::string val; m_dataFile->assignFromKey(val, hashes, kKeyHashVersionRelease))
				m_hashVersion = std::move(val);

			if (std::string val; m_dataFile->assignFromKey(val, hashes, kKeyHashVersionDebug))
				m_hashVersionDebug = std::move(val);

			if (hashes.contains(kKeyHashExtra))
			{
				auto& extra = hashes.at(kKeyHashExtra);
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

	if (std::string rawValue; m_dataFile->assignFromKey(rawValue, rootNode, kKeyLastChaletJsonWriteTime))
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
	if (m_buildHash.empty())
		return true;

	for (auto& [_, sourceCache] : m_sourceCaches)
	{
		m_dirty |= sourceCache->dirty();
	}

	if (m_dirty && m_dataFile != nullptr)
	{
		Json rootNode = Json::object();

		rootNode[kKeyHashes] = Json::object();

		if (!m_buildHash.empty())
			rootNode[kKeyHashes][kKeyHashBuild] = m_buildHash;

		if (!m_hashTheme.empty())
			rootNode[kKeyHashes][kKeyHashTheme] = m_hashTheme;

		if (!m_hashVersionDebug.empty())
			rootNode[kKeyHashes][kKeyHashVersionDebug] = m_hashVersionDebug;

		if (!m_hashVersion.empty())
			rootNode[kKeyHashes][kKeyHashVersionRelease] = m_hashVersion;

		rootNode[kKeyHashes][kKeyHashExtra] = Json::array();
		for (auto& hash : m_extraHashes)
		{
			rootNode[kKeyHashes][kKeyHashExtra].push_back(hash);
		}

		rootNode[kKeyLastChaletJsonWriteTime] = std::to_string(m_lastBuildFileWrite);

		if (!m_dataFile->json.contains(kKeyBuilds))
			rootNode[kKeyBuilds] = Json::object();
		else
			rootNode[kKeyBuilds] = m_dataFile->json.at(kKeyBuilds);

		for (auto& [id, sourceCache] : m_sourceCaches)
		{
			rootNode[kKeyBuilds][id] = sourceCache->asJson(kKeyBuildLastBuilt, kKeyBuildNative, kKeyBuildFiles, kKeyDataCache);
		}

		m_dataFile->setContents(std::move(rootNode));
		m_dataFile->setDirty(true);

		m_dataFile->save(0);
		// m_dataFile->save();

		m_dirty = false;

		return true;
	}

	return true;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::loadExternalDependencies(const std::string& inPath)
{
	UNUSED(inPath);

	m_externalDependencyCachePath = fmt::format("{}/.chalet_git", inPath);
	return m_externalDependencies.loadFromFilename(m_externalDependencyCachePath);
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::saveExternalDependencies()
{
	m_dirty |= m_externalDependencies.dirty();

	chalet_assert(!m_externalDependencyCachePath.empty(), "");

	if (m_externalDependencies.dirty())
	{
		std::ofstream(m_externalDependencyCachePath) << m_externalDependencies.asString()
													 << std::endl;
	}

	if (m_externalDependencies.empty())
	{
		if (Commands::pathExists(m_externalDependencyCachePath))
			Commands::remove(m_externalDependencyCachePath);
	}

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
bool WorkspaceInternalCacheFile::themeChanged() const noexcept
{
	return m_themeChanged;
}

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
std::string WorkspaceInternalCacheFile::getAppVersionHash(std::string appPath)
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

/*****************************************************************************/
StringList WorkspaceInternalCacheFile::getCacheIdsForRemoval() const
{
	StringList ret;
	ret.emplace_back(String::getPathFilename(m_filename));

	for (auto& id : m_extraHashes)
	{
		if (List::contains(m_doNotRemoves, id))
			continue;

		List::addIfDoesNotExist(ret, id);
	}

	for (auto& [id, _] : m_sourceCaches)
	{
		if (List::contains(m_doNotRemoves, id))
			continue;

		List::addIfDoesNotExist(ret, id);
	}

	auto& rootNode = m_dataFile->json;
	if (rootNode.contains(kKeyBuilds))
	{
		auto& builds = rootNode.at(kKeyBuilds);
		if (builds.is_object())
		{
			for (auto [id, _] : builds.items())
			{
				if (List::contains(m_doNotRemoves, id))
					continue;

				List::addIfDoesNotExist(ret, id);
			}
		}
	}

	return ret;
}
}
