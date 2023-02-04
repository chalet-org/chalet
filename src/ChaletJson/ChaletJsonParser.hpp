/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CHALET_JSON_PARSER_HPP
#define CHALET_CHALET_JSON_PARSER_HPP

#include "ChaletJson/ChaletJsonParserAdapter.hpp"
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
struct CentralState;
class BuildState;

struct BundleTarget;
struct CMakeTarget;
struct SourceTarget;
struct ProcessBuildTarget;
struct ScriptBuildTarget;
struct SubChaletTarget;

struct IDistTarget;
struct BundleTarget;
struct ScriptDistTarget;
struct ProcessDistTarget;
struct BundleArchiveTarget;
struct MacosDiskImageTarget;

struct ChaletJsonParser
{
	explicit ChaletJsonParser(CentralState& inCentralState, BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(ChaletJsonParser);
	~ChaletJsonParser();

	bool serialize();

private:
	bool serializeFromJsonRoot(const Json& inJson);

	bool parseRoot(const Json& inNode) const;
	bool parsePlatformRequires(const Json& inNode) const;

	bool parseTargets(const Json& inNode);
	bool parseSourceTarget(SourceTarget& outTarget, const Json& inNode) const;
	bool parseScriptTarget(ScriptBuildTarget& outTarget, const Json& inNode) const;
	bool parseSubChaletTarget(SubChaletTarget& outTarget, const Json& inNode) const;
	bool parseCMakeTarget(CMakeTarget& outTarget, const Json& inNode) const;
	bool parseProcessTarget(ProcessBuildTarget& outTarget, const Json& inNode) const;
	bool parseRunTargetProperties(IBuildTarget& outTarget, const Json& inNode) const;
	bool parseCompilerSettingsCxx(SourceTarget& outTarget, const Json& inNode) const;
	bool parseSourceTargetMetadata(SourceTarget& outTarget, const Json& inNode) const;

	bool parseDistribution(const Json& inNode) const;
	bool parseDistributionScript(ScriptDistTarget& outTarget, const Json& inNode) const;
	bool parseDistributionProcess(ProcessDistTarget& outTarget, const Json& inNode) const;
	bool parseDistributionArchive(BundleArchiveTarget& outTarget, const Json& inNode) const;
	bool parseDistributionBundle(BundleTarget& outTarget, const Json& inNode, const Json& inRoot) const;
	bool parseMacosDiskImage(MacosDiskImageTarget& outTarget, const Json& inNode) const;

	std::optional<bool> parseTargetCondition(IBuildTarget& outTarget, const Json& inNode) const;
	std::optional<bool> parseTargetCondition(IDistTarget& outTarget, const Json& inNode) const;

	bool validBuildRequested() const;
	bool validRunTargetRequested() const;
	bool validRunTargetRequestedFromInput();
	std::optional<bool> conditionIsValid(IBuildTarget& outTarget, const std::string& inContent) const;
	std::optional<bool> conditionIsValid(const std::string& inContent) const;
	ConditionResult checkConditionVariable(IBuildTarget& outTarget, const std::string& inString, const std::string& key, const std::string& value, bool negate) const;
	ConditionResult checkConditionVariable(const std::string& inString, const std::string& key, const std::string& value, bool negate) const;

	template <typename T>
	bool valueMatchesSearchKeyPattern(T& outVariable, const Json& inNode, const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const;

	JsonFile& m_chaletJson;
	CentralState& m_centralState;
	BuildState& m_state;

	HeapDictionary<SourceTarget> m_abstractSourceTarget;

	ChaletJsonParserAdapter m_adapter;

	const StringList kValidPlatforms;

	StringList m_notPlatforms;
	// StringList m_notToolchains;
	std::string m_platform;
	// std::string m_toolchain;
};
}

#include "ChaletJson/ChaletJsonParser.inl"

#endif // CHALET_CHALET_JSON_PARSER_HPP
