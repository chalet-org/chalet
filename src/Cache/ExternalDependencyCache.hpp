/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_EXTERNAL_DEPENDENCY_CACHE_HPP
#define CHALET_EXTERNAL_DEPENDENCY_CACHE_HPP

namespace chalet
{
class ExternalDependencyCache
{
	using StringMap = std::unordered_map<std::string, std::string>;

public:
	ExternalDependencyCache() = default;

	bool loadFromFilename(const std::string& inFilename);

	bool dirty() const noexcept;
	bool empty() const noexcept;
	bool contains(const std::string& inKey);

	const std::string& get(const std::string& inKey);
	void set(const std::string& inKey, std::string&& inValue);

	void emplace(const std::string& inKey, const std::string& inValue);
	void emplace(const std::string& inKey, std::string&& inValue);

	void erase(const std::string& inKey);

	StringMap::iterator begin() noexcept;
	StringMap::iterator end() noexcept;

	std::string asString() const;

private:
	StringMap m_cache;

	bool m_dirty = false;
};
}

#endif // CHALET_EXTERNAL_DEPENDENCY_CACHE_HPP
