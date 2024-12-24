/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "ChaletJson/ChaletJsonParserAdapter.hpp"
#include "Libraries/Json.hpp"
#include "Platform/Platform.hpp"
#include "Process/Environment.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonNodeReadStatus.hpp"

namespace chalet
{
struct CentralState;
class BuildState;
struct BuildFileChecker;

struct IBuildTarget;
struct SourceTarget;
struct SubChaletTarget;
struct CMakeTarget;
struct ScriptBuildTarget;
struct ProcessBuildTarget;
struct ValidationBuildTarget;

struct IDistTarget;
struct BundleTarget;
struct BundleArchiveTarget;
struct MacosDiskImageTarget;
struct ScriptDistTarget;
struct ProcessDistTarget;
struct ValidationDistTarget;

struct SourcePackage;

struct ChaletJsonParser
{
	explicit ChaletJsonParser(BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(ChaletJsonParser);
	~ChaletJsonParser();

	bool serialize();
	bool readPackagesIfAvailable(const std::string& inFilename, const std::string& inRoot);

private:
	friend struct BuildFileChecker;

	const StringList& getBuildTargets() const;

	bool serializeFromJsonRoot(const Json& inJson);

	bool parseRoot(const Json& inNode) const;
	bool parsePlatformRequires(const Json& inNode) const;

	bool parsePackage(const Json& inNode, const std::string& inRoot = std::string()) const;
	bool parsePackageTarget(SourcePackage& outTarget, const Json& inNode) const;
	bool parsePackageSettingsCxx(SourcePackage& outTarget, const Json& inNode) const;

	bool parseTargets(const Json& inNode);
	bool parseSourceTarget(SourceTarget& outTarget, const Json& inNode) const;
	bool parseSubChaletTarget(SubChaletTarget& outTarget, const Json& inNode) const;
	bool parseCMakeTarget(CMakeTarget& outTarget, const Json& inNode) const;
	bool parseScriptTarget(ScriptBuildTarget& outTarget, const Json& inNode) const;
	bool parseProcessTarget(ProcessBuildTarget& outTarget, const Json& inNode) const;
	bool parseValidationTarget(ValidationBuildTarget& outTarget, const Json& inNode) const;
	bool parseRunTargetProperties(IBuildTarget& outTarget, const Json& inNode) const;
	bool parseCompilerSettingsCxx(SourceTarget& outTarget, const Json& inNode) const;
	bool parseSourceTargetMetadata(SourceTarget& outTarget, const Json& inNode) const;

	bool parseDistribution(const Json& inNode) const;
	bool parseDistributionArchive(BundleArchiveTarget& outTarget, const Json& inNode) const;
	bool parseDistributionBundle(BundleTarget& outTarget, const Json& inNode, const Json& inRoot) const;
	bool parseMacosDiskImage(MacosDiskImageTarget& outTarget, const Json& inNode) const;
	bool parseDistributionScript(ScriptDistTarget& outTarget, const Json& inNode) const;
	bool parseDistributionProcess(ProcessDistTarget& outTarget, const Json& inNode) const;
	bool parseDistributionValidation(ValidationDistTarget& outTarget, const Json& inNode) const;

	std::optional<bool> parseTargetCondition(IBuildTarget& outTarget, const Json& inNode) const;
	std::optional<bool> parseTargetCondition(IDistTarget& outTarget, const Json& inNode) const;

	bool validBuildRequested() const;
	std::string getValidRunTargetFromInput() const;
	std::optional<bool> conditionIsValid(const std::string& inTargetName, const std::string& inContent) const;
	std::optional<bool> conditionIsValid(const std::string& inContent) const;
	ConditionResult checkConditionVariable(const std::string& inTargetName, const std::string& inString, const std::string& key, const std::string& value, bool negate) const;
	ConditionResult checkConditionVariable(const std::string& inString, const std::string& key, const std::string& value, bool negate) const;

	template <typename T>
	bool valueMatchesSearchKeyPattern(T& outVariable, const Json& inNode, const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const;
	bool valueMatchesSearchKeyPattern(const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const;

	JsonFile& m_chaletJson;
	BuildState& m_state;

	HeapDictionary<SourceTarget> m_abstractSourceTarget;

	ChaletJsonParserAdapter m_adapter;

	const StringList kValidPlatforms;

	StringList m_notPlatforms;
	std::string m_platform;
	mutable StringList m_buildTargets;

	bool m_isWebPlatform = false;
};
}

#include "ChaletJson/ChaletJsonParser.inl"
