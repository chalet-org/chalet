/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Cache/SourceCache.hpp"

#include "System/Files.hpp"
#include "Utility/EnumIterator.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{

/*****************************************************************************/
SourceCache::SourceCache(const std::time_t inLastBuildTime) :
	m_initializedTime(inLastBuildTime),
	m_lastBuildTime(inLastBuildTime)
{
}

/*****************************************************************************/
bool SourceCache::dirty() const
{
	return m_dirty;
}

/*****************************************************************************/
Json SourceCache::asJson() const
{
	Json ret = Json::object();

	time_t lastBuilt = m_dirty ? m_initializedTime : m_lastBuildTime;
	ret[CacheKeys::BuildLastBuilt] = std::to_string(++lastBuilt);

	ret[CacheKeys::BuildLastBuildStrategy] = static_cast<i32>(m_lastBuildStrategy);

	if (!m_dataCache.empty())
	{
		ret[CacheKeys::HashDataCache] = Json::object();

		for (auto& [key, value] : m_dataCache)
		{
			ret[CacheKeys::HashDataCache][key] = value;
		}
	}

	if (!m_fileCache.empty())
	{
		ret[CacheKeys::BuildFiles] = Json::array();

		for (auto& hash : m_fileCache)
		{
			ret[CacheKeys::BuildFiles].push_back(hash);
		}
	}

	return ret;
}

/*****************************************************************************/
bool SourceCache::buildStrategyChanged() const noexcept
{
	return m_buildStrategyChanged;
}

/*****************************************************************************/
void SourceCache::addDataCache(const std::string& inKey, const std::string& inValue)
{
	m_dataCache[inKey] = inValue;
	m_dirty = true;
}

/*****************************************************************************/
void SourceCache::addDataCache(const std::string& inKey, std::string&& inValue)
{
	m_dataCache[inKey] = std::move(inValue);
	m_dirty = true;
}

/*****************************************************************************/
void SourceCache::addDataCache(const std::string& inKey, const bool inValue)
{
	m_dataCache[inKey] = inValue ? "1" : "0";
	m_dirty = true;
}

/*****************************************************************************/
bool SourceCache::dataCacheValueChanged(const std::string& inHash, const std::string& inValue)
{
	auto& value = getDataCacheValue(inHash);
	bool result = value != inValue;
	if (result)
	{
		m_dataCache[inHash] = inValue;
		m_dirty = true;
	}

	return result;
}

/*****************************************************************************/
bool SourceCache::dataCacheValueIsFalse(const std::string& inHash)
{
	auto& value = getDataCacheValue(inHash);
	bool result = value.empty() || String::equals("0", value);
	return result;
}

/*****************************************************************************/
bool SourceCache::fileChangedOrDoesNotExist(const std::string& inFile) const
{
	if (inFile.empty())
		return false;

	auto lastWrite = Files::getLastWriteTime(inFile);
	if (lastWrite == 0)
		return true;

	return lastWrite > m_lastBuildTime;
}

/*****************************************************************************/
bool SourceCache::fileChangedOrDoesNotExist(const std::string& inFile, const std::string& inDependency) const
{
	bool depDoesNotExist = !inDependency.empty() && !Files::pathExists(inDependency);
	return depDoesNotExist || fileChangedOrDoesNotExist(inFile);
}

/*****************************************************************************/
bool SourceCache::fileChangedOrDoesNotExistWithCache(const std::string& inFile)
{
	bool result = fileChangedOrDoesNotExist(inFile);
	if (!result)
	{
		auto hash = Hash::uint64(inFile);
		if (List::contains(m_fileCache, hash))
			result = true;
	}

	return result;
}

/*****************************************************************************/
void SourceCache::addOrRemoveFileCache(const std::string& inFile, const bool inResult)
{
	auto hash = Hash::uint64(inFile);
	if (inResult)
	{
		List::removeIfExists(m_fileCache, hash);
	}
	else
	{
		List::addIfDoesNotExist(m_fileCache, std::move(hash));
	}
}

/*****************************************************************************/
bool SourceCache::updateInitializedTime()
{
	m_initializedTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	m_dirty = true;

	return true;
}

/*****************************************************************************/
void SourceCache::setLastBuildStrategy(const i32 inValue, const bool inCheckChanges) noexcept
{
	if (inValue < static_cast<i32>(StrategyType::None) || inValue >= static_cast<i32>(StrategyType::Count))
	{
		m_lastBuildStrategy = StrategyType::None;
		m_buildStrategyChanged = true;
	}
	else
	{
		auto value = static_cast<StrategyType>(inValue);
		if (inCheckChanges && !m_buildStrategyChanged)
			m_buildStrategyChanged = value != m_lastBuildStrategy;

		m_lastBuildStrategy = value;
	}
}

/*****************************************************************************/
bool SourceCache::canRemoveCachedFolder() const noexcept
{
	return m_lastBuildStrategy != StrategyType::Native
		&& m_lastBuildStrategy != StrategyType::MSBuild
		&& m_lastBuildStrategy != StrategyType::XcodeBuild;
}

/*****************************************************************************/
const std::string& SourceCache::getDataCacheValue(const std::string& inKey) noexcept
{
	if (m_dataCache.find(inKey) == m_dataCache.end())
	{
		m_dataCache[inKey] = std::string();
	}

	return m_dataCache.at(inKey);
}

/*****************************************************************************/
void SourceCache::addToFileCache(size_t inValue)
{
	m_fileCache.emplace_back(std::move(inValue));
}

}
