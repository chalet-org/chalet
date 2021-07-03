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

		auto [it, success] = m_sourceCaches.emplace(inId, std::make_unique<SourceCache>(m_lastWrites, m_initializedTime, m_initializedTime));
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
bool WorkspaceInternalCacheFile::initialize(const std::string& inFilename, const std::string& inBuildFile)
{
	m_filename = inFilename;
	m_initializedTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	m_lastBuildFileWrite = Commands::getLastWriteTime(inBuildFile);

	auto onError = []() -> bool {
		Diagnostic::error("Invalid key found in cache. Aborting.");
		return false;
	};

	chalet_assert(m_initializedTime != 0, "");

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
					auto split = String::split(line, '|');
					if (split.size() != 2)
						return onError();

					const auto& buildHash = split.front();
					const auto& lastBuildRaw = split.back();

					if (buildHash.empty() || lastBuildRaw.empty())
						return onError();

					std::time_t lastBuildFileWrite = strtoll(lastBuildRaw.c_str(), NULL, 0);
					m_buildHash = std::move(buildHash);
					m_buildFileChanged = lastBuildFileWrite != m_lastBuildFileWrite;
					break;
				}
				case 1: {
					m_hashTheme = std::move(line);
					break;
				}
				case 2: {
					if (String::contains('|', line))
					{
						auto split = String::split(line, '|');
						if (split.size() != 2)
							return onError();

						m_hashVersion = std::move(split.front());
						m_hashVersionDebug = std::move(split.back());
					}
					else
					{
						m_hashVersion = std::move(line);
					}
					break;
				}
				default: {
					if (String::startsWith('#', line))
					{
						std::string id = line.substr(1);
						if (id.empty())
							return onError();

						addExtraHash(std::move(id));
					}
					else if (String::startsWith('@', line))
					{
						auto split = String::split(line.substr(1), '|');
						if (split.size() != 2)
							return onError();

						const auto& id = split.front();
						const auto& lastBuildRaw = split.back();

						if (id.empty() || lastBuildRaw.empty())
							return onError();

						if (m_sourceCaches.find(id) != m_sourceCaches.end())
						{
							Diagnostic::error("Duplicate key found in cache: {}", id);
							return false;
						}

						std::time_t lastBuild = strtoll(lastBuildRaw.c_str(), NULL, 0);
						auto [it, success] = m_sourceCaches.emplace(id, std::make_unique<SourceCache>(m_lastWrites, m_initializedTime, lastBuild));
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
		contents += fmt::format("{}|{}\n", m_buildHash, m_lastBuildFileWrite);
		contents += fmt::format("{}\n", m_hashTheme);

		if (!m_hashVersionDebug.empty())
			contents += fmt::format("{}|{}\n", m_hashVersion, m_hashVersionDebug);
		else
			contents += fmt::format("{}\n", m_hashVersion);

		for (auto& hash : m_extraHashes)
		{
			contents += fmt::format("#{}\n", hash);
		}

		for (auto& [id, sourceCache] : m_sourceCaches)
		{
			contents += sourceCache->asString(id);
		}

		if (m_sourceCaches.size() > 0)
		{
			SourceCache* sourceCache = m_sourceCaches.begin()->second.get();
			for (auto& [file, fileData] : m_lastWrites)
			{
				if (!Commands::pathExists(file))
					continue;

				if (fileData.needsUpdate)
					sourceCache->makeUpdate(file, fileData);

				contents += fmt::format("{}|{}\n", fileData.lastWrite, file);
			}
			sourceCache = nullptr;
		}

		m_dirty = false;

		return Commands::createFileWithContents(m_filename, contents);
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

		ret.push_back(id);
	}

	for (auto& [id, _] : m_sourceCaches)
	{
		if (List::contains(m_doNotRemoves, id))
			continue;

		ret.push_back(id);
	}

	return ret;
}
}
