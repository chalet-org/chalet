/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Cache/ExternalDependencyCache.hpp"

#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
bool ExternalDependencyCache::loadFromPath(const std::string& inPath)
{
	m_filename = fmt::format("{}/.chalet_git", inPath);

	JsonFile jsonFile(m_filename);
	if (!jsonFile.load(false))
		jsonFile.resetAndSave();

	if (!jsonFile.json.is_object())
		jsonFile.resetAndSave();

	for (const auto& [key, value] : jsonFile.json.items())
	{
		m_cache.emplace(key, value);
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

	if (m_cache.empty())
	{
		if (Commands::pathExists(m_filename))
			Commands::remove(m_filename);
	}
	else
	{
		JsonFile jsonFile(m_filename);
		jsonFile.json = Json::object();
		for (auto& [key, value] : m_cache)
		{
			jsonFile.json[key] = value;
		}
		jsonFile.setDirty(true);
		jsonFile.save();
	}

	return true;
}

/*****************************************************************************/
const Json& ExternalDependencyCache::get(const std::string& inKey)
{
	return m_cache.at(inKey);
}

/*****************************************************************************/
void ExternalDependencyCache::set(const std::string& inKey, Json&& inValue)
{
	m_cache[inKey] = std::move(inValue);
	m_dirty = true;
}

/*****************************************************************************/
void ExternalDependencyCache::emplace(const std::string& inKey, const Json& inValue)
{
	m_cache.emplace(inKey, inValue);
	m_dirty = true;
}

/*****************************************************************************/
void ExternalDependencyCache::emplace(const std::string& inKey, Json&& inValue)
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
}
