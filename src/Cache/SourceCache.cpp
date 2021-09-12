/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Cache/SourceCache.hpp"

#include "Terminal/Commands.hpp"

namespace chalet
{
/*****************************************************************************/
SourceCache::SourceCache(const std::time_t inLastBuildTime) :
	m_initializedTime(inLastBuildTime),
	m_lastBuildTime(inLastBuildTime)
{
}

/*****************************************************************************/
bool SourceCache::native() const noexcept
{
	return m_native;
}

/*****************************************************************************/
void SourceCache::setNative(const bool inValue) noexcept
{
	m_native = inValue;
}

/*****************************************************************************/
bool SourceCache::dirty() const
{
	return m_dirty;
}

/*****************************************************************************/
Json SourceCache::asJson(const std::string& kKeyBuildLastBuilt, const std::string& kKeyBuildNative, const std::string& kKeyBuildFiles) const
{
	Json ret = Json::object();

	ret[kKeyBuildLastBuilt] = std::to_string(m_dirty ? m_initializedTime : m_lastBuildTime);

	if (m_native)
		ret[kKeyBuildNative] = true;

	ret[kKeyBuildFiles] = Json::object();

	for (auto& [file, data] : m_lastWrites)
	{
		if (!Commands::pathExists(file))
			continue;

		if (data.needsUpdate)
			forceUpdate(file, data);

		ret[kKeyBuildFiles][file] = std::to_string(data.lastWrite);
	}

	return ret;
}

/*****************************************************************************/
bool SourceCache::updateInitializedTime(const std::time_t inTime)
{
	// if (!m_dirty)
	// 	return false;

	if (inTime == 0)
		m_initializedTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	else
		m_initializedTime = inTime;

	return true;
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

void SourceCache::addLastWrite(std::string inFile, const std::time_t inLastWrite)
{
	auto& fileData = m_lastWrites[std::move(inFile)];
	fileData.lastWrite = inLastWrite;
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
	return fileChangedOrDoesNotExist(inFile) || fileChangedOrDoesNotExist(inDependency);
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
void SourceCache::forceUpdate(const std::string& inFile, LastWrite& outFileData) const
{
	auto lastWrite = Commands::getLastWriteTime(inFile);
	if (lastWrite == 0)
		lastWrite = m_initializedTime;

	outFileData.lastWrite = lastWrite;
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
