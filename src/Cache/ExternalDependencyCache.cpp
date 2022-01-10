/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Cache/ExternalDependencyCache.hpp"

#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool ExternalDependencyCache::loadFromPath(const std::string& inPath)
{
	m_filename = fmt::format("{}/.chalet_git", inPath);

	std::ifstream input(m_filename);
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
bool ExternalDependencyCache::contains(const std::string& inKey)
{
	return m_cache.find(inKey) != m_cache.end();
}

/*****************************************************************************/
bool ExternalDependencyCache::save() const
{
	chalet_assert(!m_filename.empty(), "");

	if (!m_dirty || m_filename.empty())
		return false;

	std::ofstream(m_filename) << this->asString()
							  << std::endl;

	if (m_cache.empty())
	{
		if (Commands::pathExists(m_filename))
			Commands::remove(m_filename);
	}

	return true;
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
void ExternalDependencyCache::emplace(const std::string& inKey, const std::string& inValue)
{
	m_cache.emplace(inKey, inValue);
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
StringList ExternalDependencyCache::getKeys(const std::function<bool(const std::string&)>& onWhere) const
{
	StringList ret;

	std::for_each(m_cache.begin(), m_cache.end(), [&ret, &onWhere](auto& item) {
		const auto& [key, _] = item;
		if (onWhere(key))
			ret.push_back(key);
	});

	return ret;
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
