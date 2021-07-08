/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Cache/SourceCache.hpp"

#include "Terminal/Commands.hpp"

namespace chalet
{
/*****************************************************************************/
SourceCache::SourceCache(SourceLastWriteMap& inLastWrites, const std::time_t& inInitializedTime, const std::time_t inLastBuildTime) :
	m_lastWrites(inLastWrites),
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
std::string SourceCache::asString(const std::string& inId) const
{
	std::string ret;

	ret += fmt::format("@{}\t{}\n", inId, m_dirty ? m_initializedTime : m_lastBuildTime);

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
		makeUpdate(inFile, fileData);

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
		makeUpdate(inFile, fileData);

	return fileData.lastWrite > m_lastBuildTime;
}

/*****************************************************************************/
bool SourceCache::fileChangedOrDependantChanged(const std::string& inFile, const std::string& inDependency) const
{
	if (!Commands::pathExists(inFile) || fileChangedOrDoesNotExist(inDependency))
	{
		m_lastWrites[inFile].lastWrite = m_initializedTime;
		m_lastWrites[inFile].needsUpdate = true;
		m_dirty = true;
		return true;
	}

	auto& fileData = getLastWrite(inFile);
	if (fileData.needsUpdate)
		makeUpdate(inFile, fileData);

	return fileData.lastWrite > m_lastBuildTime;
}

/*****************************************************************************/
void SourceCache::makeUpdate(const std::string& inFile, LastWrite& outFileData) const
{
	auto lastWrite = Commands::getLastWriteTime(inFile);
	if (lastWrite >= m_initializedTime)
	{
		outFileData.lastWrite = m_initializedTime;
	}
	else
	{
		if (lastWrite == 0)
			lastWrite = m_initializedTime;

		outFileData.lastWrite = lastWrite;
	}

	outFileData.needsUpdate = false;
	m_dirty = true;
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
