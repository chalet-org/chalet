/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Cache/WorkspaceInternalCacheFile.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
WorkspaceInternalCacheFile::WorkspaceInternalCacheFile(WorkspaceCache& inCache) :
	m_cache(inCache)
{
	UNUSED(m_cache);
}

/*****************************************************************************/
const std::string& WorkspaceInternalCacheFile::hashStrategy() const noexcept
{
	return m_hashStrategy;
}

void WorkspaceInternalCacheFile::setHashStrategy(std::string&& inValue) noexcept
{
	m_dirty |= m_hashStrategy != inValue;
	m_hashStrategy = std::move(inValue);
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
bool WorkspaceInternalCacheFile::setSourceCache(const std::string& inId)
{
	auto itr = m_sourceCaches.find(inId);
	if (itr != m_sourceCaches.end())
	{
		m_sources = itr->second.get();
	}
	else
	{
		auto [it, success] = m_sourceCaches.emplace(inId, std::make_unique<SourceCache>(m_initializedTime, m_lastBuildTime));
		if (!success)
		{
			Diagnostic::error("Error creating cache for {}", inId);
			return false;
		}
		m_sources = it->second.get();
	}

	return true;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::removeSourceCache(const std::string& inId)
{
	auto itr = m_sourceCaches.find(inId);
	if (itr != m_sourceCaches.end())
	{
		if (m_sources == itr->second.get())
			m_sources = nullptr;

		itr = m_sourceCaches.erase(itr);
		m_dirty = true;
		return true;
	}

	return false;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::removeExtraCache(const std::string& inId)
{
	bool result = false;
	auto itr = m_extraHashes.begin();
	for (; itr < m_extraHashes.end(); ++itr)
	{
		if (*itr == inId)
		{
			itr = m_extraHashes.erase(itr);
			m_dirty = true;
			result = true;
		}
	}

	return result;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::initialize(const std::string& inFilename)
{
	m_filename = inFilename;
	m_initializedTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

	if (Commands::pathExists(m_filename))
	{
		SourceCache* sourceCache = nullptr;
		int i = 0;
		std::ifstream input(m_filename);
		for (std::string line; std::getline(input, line);)
		{
			switch (i)
			{
				case 0: {
					m_hashWorkingDirectory = std::move(line);
					break;
				}
				case 1: {
					m_hashStrategy = std::move(line);
					break;
				}
				case 2: {
					auto splitVar = String::split(line, '|');
					if (splitVar.size() == 2)
					{
						m_hashVersion = std::move(splitVar.front());
						m_hashVersionDebug = std::move(splitVar.back());
					}
					break;
				}
				case 3: {
					m_lastBuildTime = strtoll(line.c_str(), NULL, 0);
					break;
				}
				default: {
					if (String::startsWith('#', line))
					{
						std::string id = line.substr(1);
						if (id.empty())
						{
							Diagnostic::error("Empty key found in cache. Aborting.");
							return false;
						}
						addExtraHash(std::move(id));
					}
					else if (String::startsWith('@', line))
					{
						std::string id = line.substr(1);
						if (id.empty())
						{
							Diagnostic::error("Empty key found in cache. Aborting.");
							return false;
						}
						if (m_sourceCaches.find(id) != m_sourceCaches.end())
						{
							Diagnostic::error("Duplicate key found in cache: {}", id);
							return false;
						}
						auto [it, success] = m_sourceCaches.emplace(id, std::make_unique<SourceCache>(m_initializedTime, m_lastBuildTime));
						if (!success)
						{
							Diagnostic::error("Error creating cache for {}", id);
							return false;
						}
						sourceCache = it->second.get();
					}
					else
					{

						auto splitVar = String::split(line, '|');
						if (splitVar.size() == 2 && splitVar.front().size() > 0 && splitVar.back().size() > 0)
						{
							chalet_assert(sourceCache != nullptr, "");
							if (sourceCache != nullptr)
								sourceCache->addLastWrite(std::move(splitVar.back()), splitVar.front());
						}
					}
					break;
				}
			}

			++i;
		}
		sourceCache = nullptr;
	}

	return true;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::save()
{
	m_sources = nullptr;

	if (m_filename.empty())
		return false;

	for (auto& [_, sourceCache] : m_sourceCaches)
	{
		m_dirty |= sourceCache->dirty();
	}

	if (m_dirty)
	{
		std::string contents;
		contents += fmt::format("{}\n", m_hashWorkingDirectory);
		contents += fmt::format("{}\n", m_hashStrategy);
		contents += fmt::format("{}|{}\n", m_hashVersion, m_hashVersionDebug);
		contents += fmt::format("{}\n", m_initializedTime);

		for (auto& hash : m_extraHashes)
		{
			contents += fmt::format("#{}\n", hash);
		}

		for (auto& [id, sourceCache] : m_sourceCaches)
		{
			contents += fmt::format("@{}\n", id);
			contents += sourceCache->asString();
		}

		m_dirty = false;

		return Commands::createFileWithContents(m_filename, contents);
	}

	return true;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::loadExternalDependencies(const std::string& inPath)
{
	auto hash = Hash::string("chalet_external_dependencies_cache");

	UNUSED(inPath);

	// m_externalDependencyCachePath = fmt::format("{}/{}", inPath, hash);
	m_externalDependencyCachePath = m_cache.getCachePath(hash, CacheType::Local);
	return m_externalDependencies.loadFromFilename(m_externalDependencyCachePath);
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::saveExternalDependencies()
{
	m_dirty |= m_externalDependencies.dirty();

	chalet_assert(!m_externalDependencyCachePath.empty(), "");

	if (m_externalDependencies.dirty())
	{
		// auto externalPath = m_cache.getCachePath(hash, CacheType::Local);
		std::ofstream(m_externalDependencyCachePath) << m_externalDependencies.asString()
													 << std::endl;
	}

	auto hash = String::getPathFilename(m_externalDependencyCachePath);
	if (!m_externalDependencies.empty())
	{
		addExtraHash(std::move(hash));
	}
	else
	{
		if (Commands::pathExists(m_externalDependencyCachePath))
			Commands::remove(m_externalDependencyCachePath);
	}

	return true;
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::workingDirectoryChanged() const noexcept
{
	return m_workingDirectoryChanged;
}

/*****************************************************************************/
void WorkspaceInternalCacheFile::checkIfWorkingDirectoryChanged(const std::string& inWorkingDirectory)
{
	m_workingDirectoryChanged = false;

	auto workingDirectoryHash = Hash::string(inWorkingDirectory);

	if (workingDirectoryHash != m_hashWorkingDirectory)
	{
		m_hashWorkingDirectory = std::move(workingDirectoryHash);
		m_workingDirectoryChanged = true;
		m_dirty = true;
	}
}

/*****************************************************************************/
bool WorkspaceInternalCacheFile::appVersionChanged() const noexcept
{
	return false;
	// return m_appVersionChanged;
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
StringList WorkspaceInternalCacheFile::getCacheIds() const
{
	StringList ret;
	ret.push_back(String::getPathFilename(m_filename));

	for (auto& id : m_extraHashes)
	{
		ret.push_back(id);
	}

	for (auto& [id, _] : m_sourceCaches)
	{
		ret.push_back(id);
	}

	return ret;
}
}
