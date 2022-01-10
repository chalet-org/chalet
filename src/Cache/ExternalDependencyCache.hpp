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
	using StringMap = Dictionary<std::string>;

public:
	/*struct GitCache
	{
		std::string commit;
		std::string branch;
		std::string targetCommit;
		std::string targetBranch;
		std::string targetTag;
	};*/
	ExternalDependencyCache() = default;

	bool loadFromPath(const std::string& inPath);

	bool contains(const std::string& inKey);
	bool save() const;

	const std::string& get(const std::string& inKey);
	void set(const std::string& inKey, std::string&& inValue);

	void emplace(const std::string& inKey, const std::string& inValue);
	void emplace(const std::string& inKey, std::string&& inValue);

	void erase(const std::string& inKey);

	StringList getKeys(const std::function<bool(const std::string&)>& onWhere) const;

	std::string asString() const;

private:
	std::string m_filename;
	StringMap m_cache;

	bool m_dirty = false;
};
}

#endif // CHALET_EXTERNAL_DEPENDENCY_CACHE_HPP
