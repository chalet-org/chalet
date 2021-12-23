/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_JSON_PARSER_HPP
#define CHALET_BUILD_JSON_PARSER_HPP

#include "Core/Platform.hpp"
#include "Libraries/Json.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonNodeReadStatus.hpp"

namespace chalet
{
struct BundleTarget;
struct CMakeTarget;
struct SourceTarget;
struct ProcessBuildTarget;
struct ScriptBuildTarget;
struct SubChaletTarget;
struct StatePrototype;
class BuildState;

struct BuildJsonParser
{
	explicit BuildJsonParser(StatePrototype& inPrototype, BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(BuildJsonParser);
	~BuildJsonParser();

	bool serialize();

private:
	bool serializeFromJsonRoot(const Json& inJson);

	bool parseTarget(const Json& inNode);
	bool parseSourceTarget(SourceTarget& outTarget, const Json& inNode, const bool inAbstract = false) const;
	bool parseScriptTarget(ScriptBuildTarget& outTarget, const Json& inNode) const;
	bool parseSubChaletTarget(SubChaletTarget& outTarget, const Json& inNode) const;
	bool parseCMakeTarget(CMakeTarget& outTarget, const Json& inNode) const;
	bool parseProcessTarget(ProcessBuildTarget& outTarget, const Json& inNode) const;
	bool parseTargetCondition(IBuildTarget& outTarget, const Json& inNode) const;
	bool parseRunTargetProperties(IBuildTarget& outTarget, const Json& inNode) const;
	bool parseCompilerSettingsCxx(SourceTarget& outTarget, const Json& inNode) const;

	bool validBuildRequested() const;
	bool validRunTargetRequested() const;
	bool validRunTargetRequestedFromInput() const;
	bool conditionIsValid(const std::string& inContent) const;

	template <typename T>
	bool valueMatchesSearchKeyPattern(T& outVariable, const Json& inNode, const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const;

	template <typename T>
	bool valueMatchesToolchainSearchPattern(T& outVariable, const Json& inNode, const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const;

	JsonFile& m_chaletJson;
	const std::string& m_filename;
	BuildState& m_state;

	HeapDictionary<SourceTarget> m_abstractSourceTarget;

	StringList m_notPlatforms;
	std::string m_platform;
};
}

#include "BuildJson/BuildJsonParser.inl"

#endif // CHALET_BUILD_JSON_PARSER_HPP
