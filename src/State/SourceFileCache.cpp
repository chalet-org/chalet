/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/SourceFileCache.hpp"

#include "State/BuildCache.hpp"
#include "State/BuildPaths.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
SourceFileCache::SourceFileCache(BuildCache& inCache) :
	m_cache(inCache)
{
}

/*****************************************************************************/
bool SourceFileCache::initialize()
{
	{
		auto path = m_cache.getPath("", BuildCache::Type::Local);
		auto file = Hash::string("chalet_dependencies");
		m_cache.addSourceCache(file);
		m_filename = fmt::format("{}/{}", path, file);
	}

	if (Commands::pathExists(m_filename))
	{
		int i = 0;
		std::ifstream input(m_filename);
		for (std::string line; std::getline(input, line);)
		{
			if (i == 0)
			{
				m_lastBuildTime = strtoll(line.c_str(), NULL, 0);
			}
			else
			{
				auto splitVar = String::split(line, '|');
				if (splitVar.size() == 2 && splitVar.front().size() > 0 && splitVar.back().size() > 0)
				{
					auto& file = splitVar.back();
					std::time_t lastWrite = strtoll(splitVar.front().c_str(), NULL, 0);

					auto& fileData = m_lastWrites[std::move(file)];
					fileData.lastWrite = lastWrite;
					fileData.needsUpdate = true;
				}
			}
			++i;
		}
	}

	m_initializedTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

	return true;
}

/*****************************************************************************/
bool SourceFileCache::save()
{
	if (m_filename.empty())
		return false;

	if (m_dirty)
	{
		std::string contents = fmt::format("{}\n", m_initializedTime);
		for (auto& [file, fileData] : m_lastWrites)
		{
			if (!Commands::pathExists(file))
				continue;

			if (fileData.needsUpdate)
				add(file);

			contents += fmt::format("{}|{}\n", fileData.lastWrite, file);
		}

		m_dirty = false;

		return Commands::createFileWithContents(m_filename, contents);
	}

	return true;
}

/*****************************************************************************/
bool SourceFileCache::fileChangedOrDoesNotExist(const std::string& inFile) const
{
	if (!Commands::pathExists(inFile))
	{
		m_lastWrites[inFile].lastWrite = m_initializedTime;
		m_lastWrites[inFile].needsUpdate = true;
		m_dirty = true;
		return true;
	}

	auto& fileData = getLastWrite(inFile);
	if (fileData.needsUpdate)
		return add(inFile);

	// TODO: Older file should also restat, but need to check values more closely '!=' seemed to always restat
	return fileData.lastWrite > m_lastBuildTime;
}

/*****************************************************************************/
bool SourceFileCache::add(const std::string& inFile) const
{
	auto lastWrite = Commands::getLastWriteTime(inFile);
	if (lastWrite == 0)
		return false;

	auto& fileData = getLastWrite(inFile);
	if (lastWrite >= fileData.lastWrite && !fileData.needsUpdate)
		return false;

	fileData.lastWrite = lastWrite;
	fileData.needsUpdate = false;
	m_dirty = true;

	return true;
}

/*****************************************************************************/
SourceFileCache::LastWrite& SourceFileCache::getLastWrite(const std::string& inFile) const
{
	if (m_lastWrites.find(inFile) == m_lastWrites.end())
	{
		m_lastWrites.emplace(inFile, LastWrite());
	}
	return m_lastWrites.at(inFile);
}

}
