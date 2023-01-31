/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CENTRAL_CHALET_JSON_PARSER_HPP
#define CHALET_CENTRAL_CHALET_JSON_PARSER_HPP

#include "Core/Platform.hpp"
#include "Libraries/Json.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonNodeReadStatus.hpp"

namespace chalet
{
struct CentralState;
struct JsonFile;
struct GitDependency;

struct CentralChaletJsonParser
{
	explicit CentralChaletJsonParser(CentralState& inCentralState);
	CHALET_DISALLOW_COPY_MOVE(CentralChaletJsonParser);
	~CentralChaletJsonParser();

	bool serialize() const;

private:
	bool validateAgainstSchema() const;
	bool serializeRequiredFromJsonRoot(const Json& inNode) const;

	bool parseRoot(const Json& inNode) const;
	bool parseMetadata(const Json& inNode) const;

	bool parseVariables(const Json& inNode) const;
	bool parseDefaultConfigurations(const Json& inNode) const;
	bool parseConfigurations(const Json& inNode) const;

	bool parseExternalDependencies(const Json& inNode) const;
	bool parseGitDependency(GitDependency& outDependency, const Json& inNode) const;

	CentralState& m_centralState;
	JsonFile& m_chaletJson;

	StringList m_notPlatforms;
	std::string m_platform;
};
}

#endif // CHALET_CENTRAL_CHALET_JSON_PARSER_HPP
