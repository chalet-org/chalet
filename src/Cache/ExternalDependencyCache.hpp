/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"

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

	const Json& get(const std::string& inKey);
	void set(const std::string& inKey, Json&& inValue);

	void emplace(const std::string& inKey, const Json& inValue);
	void emplace(const std::string& inKey, Json&& inValue);

	void erase(const std::string& inKey);

	StringList getKeys(const std::function<bool(const std::string&)>& onWhere) const;

private:
	std::string m_filename;
	Dictionary<Json> m_cache;

	bool m_dirty = false;
};
}

