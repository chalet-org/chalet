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
void SourceCache::setLastBuildStrategy(const StrategyType inValue, const bool inCheckChanges) noexcept
{
	if (inCheckChanges && !m_buildStrategyChanged)
		m_buildStrategyChanged = inValue != m_lastBuildStrategy;

	m_lastBuildStrategy = inValue;
}

void SourceCache::setLastBuildStrategy(const i32 inValue, const bool inCheckChanges) noexcept
{
	if (inValue < static_cast<i32>(StrategyType::None) || inValue >= static_cast<i32>(StrategyType::Count))
		return;

	if (inCheckChanges && !m_buildStrategyChanged)
		m_buildStrategyChanged = static_cast<StrategyType>(inValue) != m_lastBuildStrategy;

	m_lastBuildStrategy = static_cast<StrategyType>(inValue);
}

bool SourceCache::buildStrategyChanged() const noexcept
{
	return m_buildStrategyChanged;
}

/*****************************************************************************/
bool SourceCache::canRemoveCachedFolder() const noexcept
{
	return m_lastBuildStrategy != StrategyType::Native
		&& m_lastBuildStrategy != StrategyType::MSBuild
		&& m_lastBuildStrategy != StrategyType::XcodeBuild;
}

/*****************************************************************************/
void SourceCache::addVersion(const std::string& inExecutable, const std::string& inValue)
{
	addDataCache(inExecutable, CacheKeys::DataVersion, inValue);
}

/*****************************************************************************/
void SourceCache::addExternalRebuild(const std::string& inPath, const std::string& inValue)
{
	addDataCache(inPath, CacheKeys::DataExternalRebuild, inValue);
}

/*****************************************************************************/
const std::string& SourceCache::dataCache(const std::string& inMainKey, const char* inKey) noexcept
{
	if (m_dataCache.find(inMainKey) == m_dataCache.end())
	{
		m_dataCache[inMainKey] = Dictionary<std::string>();
	}

	auto& data = m_dataCache.at(inMainKey);
	if (data.find(inKey) == data.end())
	{
		m_dataCache[inMainKey][inKey] = std::string();
	}

	return m_dataCache.at(inMainKey).at(inKey);
}

void SourceCache::addDataCache(const std::string& inMainKey, const char* inKey, const std::string& inValue)
{
	m_dataCache[inMainKey][inKey] = inValue;
	m_dirty = true;
}

/*****************************************************************************/
void SourceCache::addToFileCache(size_t inValue)
{
	m_fileCache.emplace_back(std::move(inValue));
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
		ret[CacheKeys::DataCache] = Json::object();

		for (auto& [file, data] : m_dataCache)
		{
			if (!Files::pathExists(file))
				continue;

			if (!data.empty())
			{
				ret[CacheKeys::DataCache][file] = Json::object();
				for (auto& [key, value] : data)
				{
					if (!value.empty())
						ret[CacheKeys::DataCache][file][key] = value;
				}
			}
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
bool SourceCache::updateInitializedTime()
{
	// if (!m_dirty)
	// 	return false;

	m_initializedTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

	return true;
}

/*****************************************************************************/
bool SourceCache::isNewBuild() const
{
	return m_lastBuildTime == 0;
}

/*****************************************************************************/
bool SourceCache::fileChangedOrDoesNotExist(const std::string& inFile) const
{
	auto lastWrite = Files::getLastWriteTime(inFile);
	if (lastWrite == 0)
		lastWrite = m_initializedTime;

	return lastWrite > m_lastBuildTime;
}

/*****************************************************************************/
bool SourceCache::fileChangedOrDoesNotExist(const std::string& inFile, const std::string& inDependency) const
{
	bool depDoesNotExist = !Files::pathExists(inDependency);
	return depDoesNotExist || fileChangedOrDoesNotExist(inFile);
}

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
bool SourceCache::versionRequriesUpdate(const std::string& inFile, std::string& outExistingValue)
{
	outExistingValue = dataCache(inFile, CacheKeys::DataVersion);
	bool result = outExistingValue.empty() || fileChangedOrDoesNotExist(inFile);
	return result;
}

/*****************************************************************************/
bool SourceCache::externalRequiresRebuild(const std::string& inPath)
{
	auto value = dataCache(inPath, CacheKeys::DataExternalRebuild);
	bool result = value.empty() || String::equals('1', value);
	return result;
}

/*****************************************************************************/
void SourceCache::updateFileCache(const std::string& inFile, const bool inResult)
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
}
