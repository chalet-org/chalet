/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CACHE_JSON_PARSER_HPP
#define CHALET_CACHE_JSON_PARSER_HPP

#include "State/BuildState.hpp"
#include "Json/JsonParser.hpp"

namespace chalet
{
struct CacheJsonParser : public JsonParser
{
	explicit CacheJsonParser(BuildState& inState);

	virtual bool serialize() final;

private:
	bool makeCache();
	bool serializeFromJsonRoot(const Json& inJson);

	bool parseRoot(const Json& inNode);

	bool parseTools(const Json& inNode);
	bool parseCompilers(const Json& inNode);

	BuildState& m_state;
	const std::string& m_filename;
};
}

#endif // CHALET_CACHE_JSON_PARSER_HPP
