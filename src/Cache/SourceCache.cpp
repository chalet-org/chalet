/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Cache/SourceCache.hpp"

#include "Terminal/Commands.hpp"

namespace chalet
{
/*****************************************************************************/
SourceCache::SourceCache(const std::time_t& inInitializedTime, const std::time_t& inLastBuildTime) :
	m_initializedTime(inInitializedTime),
	m_lastBuildTime(inLastBuildTime)
{
}

/*****************************************************************************/
bool SourceCache::dirty() const
{
	return m_dirty;
}

/*****************************************************************************/
std::string SourceCache::asString() const
{
	std::string ret;

	for (auto& [file, fileData] : m_lastWrites)
	{
		if (!Commands::pathExists(file))
			continue;

		if (fileData.needsUpdate)
			add(file, fileData);

		ret += fmt::format("{}|{}\n", fileData.lastWrite, file);
	}

	return ret;
}

/*****************************************************************************/
void SourceCache::addLastWrite(std::string inFile, const std::string& inRaw)
{
	std::time_t lastWrite = strtoll(inRaw.c_str(), NULL, 0);
	auto& fileData = m_lastWrites[std::move(inFile)];
	fileData.lastWrite = lastWrite;
	fileData.needsUpdate = true;
	m_dirty = true;
}

/*****************************************************************************/
bool SourceCache::fileChangedOrDoesNotExist(const std::string& inFile) const
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
		return add(inFile, fileData);

	// TODO: Older file should also restat, but need to check values more closely '!=' seemed to always restat
	return fileData.lastWrite > m_lastBuildTime;
}

/*****************************************************************************/
bool SourceCache::fileChangedOrDoesNotExist(const std::string& inFile, const std::string& inDependency) const
{
	if (!Commands::pathExists(inFile) || !Commands::pathExists(inDependency))
	{
		m_lastWrites[inFile].lastWrite = m_initializedTime;
		m_lastWrites[inFile].needsUpdate = true;
		m_dirty = true;
		return true;
	}

	auto& fileData = getLastWrite(inFile);
	if (fileData.needsUpdate)
		return add(inFile, fileData);

	// TODO: Older file should also restat, but need to check values more closely '!=' seemed to always restat
	return fileData.lastWrite > m_lastBuildTime;
}

/*****************************************************************************/
bool SourceCache::add(const std::string& inFile, LastWrite& outFileData) const
{
	auto lastWrite = Commands::getLastWriteTime(inFile);
	if (lastWrite == 0)
		lastWrite = m_initializedTime;

	outFileData.lastWrite = lastWrite;
	outFileData.needsUpdate = false;
	m_dirty = true;

	return true;
}

/*****************************************************************************/
LastWrite& SourceCache::getLastWrite(const std::string& inFile) const
{
	if (m_lastWrites.find(inFile) == m_lastWrites.end())
	{
		m_lastWrites.emplace(inFile, LastWrite());
	}
	return m_lastWrites.at(inFile);
}

}
