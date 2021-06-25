/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Cache/ExternalDependencyCache.hpp"

#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool ExternalDependencyCache::loadFromFilename(const std::string& inFilename)
{
	std::ifstream input(inFilename);
	for (std::string line; std::getline(input, line);)
	{
		auto split = String::split(line, '|');
		if (split.size() == 2 && !split.front().empty() && !split.back().empty())
		{
			m_cache.emplace(std::move(split.back()), std::move(split.front()));
		}
	}

	return true;
}

/*****************************************************************************/
bool ExternalDependencyCache::dirty() const noexcept
{
	return m_dirty;
}

/*****************************************************************************/
bool ExternalDependencyCache::contains(const std::string& inKey)
{
	return m_cache.find(inKey) != m_cache.end();
}

/*****************************************************************************/
const std::string& ExternalDependencyCache::get(const std::string& inKey)
{
	return m_cache.at(inKey);
}

/*****************************************************************************/
void ExternalDependencyCache::set(const std::string& inKey, std::string&& inValue)
{
	m_cache[inKey] = std::move(inValue);
	m_dirty = true;
}

/*****************************************************************************/
void ExternalDependencyCache::emplace(const std::string& inKey, std::string&& inValue)
{
	m_cache.emplace(inKey, std::move(inValue));
	m_dirty = true;
}

/*****************************************************************************/
void ExternalDependencyCache::erase(const std::string& inKey)
{
	m_cache.erase(inKey);
	m_dirty = true;
}

/*****************************************************************************/
ExternalDependencyCache::StringMap::iterator ExternalDependencyCache::begin() noexcept
{
	return m_cache.begin();
}

/*****************************************************************************/
ExternalDependencyCache::StringMap::iterator ExternalDependencyCache::end() noexcept
{
	return m_cache.end();
}

/*****************************************************************************/
std::string ExternalDependencyCache::asString() const
{
	std::string ret;
	for (auto& [key, value] : m_cache)
	{
		ret += fmt::format("{}|{}\n", value, key);
	}

	return ret;
}
}
