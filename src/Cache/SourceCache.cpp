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
	kDataVersion("v"),
	kDataArch("a"),
	m_initializedTime(inLastBuildTime),
	m_lastBuildTime(inLastBuildTime)
{
}

/*****************************************************************************/
bool SourceCache::native() const noexcept
{
	return m_native;
}

void SourceCache::setNative(const bool inValue) noexcept
{
	m_native = inValue;
}

/*****************************************************************************/
void SourceCache::addVersion(const std::string& inExecutable, const std::string& inValue)
{
	addDataCache(inExecutable, kDataVersion, inValue);
}

/*****************************************************************************/
void SourceCache::addArch(const std::string& inExecutable, const std::string& inValue)
{
	addDataCache(inExecutable, kDataArch, inValue);
}

/*****************************************************************************/
const std::string& SourceCache::dataCache(const std::string& inMainKey, const std::string& inKey) noexcept
{
	if (m_dataCache.find(inMainKey) == m_dataCache.end())
	{
		m_dataCache[inMainKey] = {};
	}

	auto& data = m_dataCache.at(inMainKey);
	if (data.find(inKey) == data.end())
	{
		m_dataCache[inMainKey][inKey] = std::string();
	}

	return m_dataCache.at(inMainKey).at(inKey);
}

void SourceCache::addDataCache(const std::string& inMainKey, const std::string& inKey, const std::string& inValue)
{
	m_dataCache[inMainKey][inKey] = inValue;
	m_dirty = true;
}

/*****************************************************************************/
bool SourceCache::dirty() const
{
	return m_dirty;
}

/*****************************************************************************/
Json SourceCache::asJson(const std::string& kKeyBuildLastBuilt, const std::string& kKeyBuildNative, const std::string& kKeyBuildFiles, const std::string& kKeyDataCache) const
{
	Json ret = Json::object();

	time_t lastBuilt = m_dirty ? m_initializedTime : m_lastBuildTime;
	ret[kKeyBuildLastBuilt] = std::to_string(++lastBuilt);

	if (m_native)
		ret[kKeyBuildNative] = true;

	if (!m_dataCache.empty())
	{
		ret[kKeyDataCache] = Json::object();

		for (auto& [file, data] : m_dataCache)
		{
			if (!Commands::pathExists(file))
				continue;

			if (!data.empty())
			{
				ret[kKeyDataCache][file] = Json::object();
				for (auto& [key, value] : data)
				{
					if (!value.empty())
						ret[kKeyDataCache][file][key] = value;
				}
			}
		}
	}

	if (!m_lastWrites.empty())
	{
		ret[kKeyBuildFiles] = Json::object();

		for (auto& [file, data] : m_lastWrites)
		{
			if (!Commands::pathExists(file))
				continue;

			if (data.needsUpdate)
				makeUpdate(file, data);

			if (m_dataCache.find(file) != m_dataCache.end())
				ret[kKeyDataCache][file][kKeyBuildFiles] = std::to_string(data.lastWrite);
			else
				ret[kKeyBuildFiles][file] = std::to_string(data.lastWrite);
		}

		if (ret[kKeyBuildFiles].empty())
			ret.erase(kKeyBuildFiles);
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
bool SourceCache::versionRequriesUpdate(const std::string& inFile, std::string& outExistingValue)
{
	outExistingValue = dataCache(inFile, kDataVersion);
	bool result = outExistingValue.empty() || fileChangedOrDoesNotExist(inFile);
	return result;
}

/*****************************************************************************/
bool SourceCache::archRequriesUpdate(const std::string& inFile, std::string& outExistingValue)
{
	outExistingValue = dataCache(inFile, kDataArch);
	bool result = outExistingValue.empty() || fileChangedOrDoesNotExist(inFile);
	return result;
}

/*****************************************************************************/
void SourceCache::makeUpdate(const std::string& inFile, LastWrite& outFileData) const
{
	auto lastWrite = Commands::getLastWriteTime(inFile);
	if (lastWrite > m_initializedTime)
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
