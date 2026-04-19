/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "ChaletJson/PropertyConditionMatcher.hpp"
#include "Libraries/Json.hpp"
#include "Platform/Platform.hpp"
#include "Process/Environment.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/TriBool.hpp"
#include "Json/IJsonFileReader.hpp"
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
struct MesonTarget;
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

struct ChaletJsonFile final : public IJsonFileReader
{
	static bool read(BuildState& inState);
	static bool readPackagesIfAvailable(BuildState& inState, const std::string& inFilename, const std::string& inRoot, StringList& outTargets);

private:
	friend struct BuildFileChecker;

	explicit ChaletJsonFile(BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(ChaletJsonFile);
	~ChaletJsonFile();

	virtual bool readFrom(JsonFile& inJsonFile) final;

	bool readPackagesIfAvailable(const std::string& inFilename, const std::string& inRoot, StringList& outTargets);

	const StringList& getBuildTargets() const;

	bool readFromRoot(const Json& inJson, const std::string& inFilename);

	bool readFromMiscellaneous(const Json& inNode) const;
	bool readFromPlatformRequires(const Json& inNode) const;

	bool readFromPackage(const Json& inNode, const std::string& inFilename, const std::string& inRoot = std::string()) const;
	bool readFromPackageTarget(SourcePackage& outTarget, const Json& inNode) const;
	bool readFromPackageSettingsCxx(SourcePackage& outTarget, const Json& inNode) const;

	bool readFromTargets(const Json& inNode, const std::string& inFilename);
	bool readFromSourceTarget(SourceTarget& outTarget, const Json& inNode) const;
	bool readFromSubChaletTarget(SubChaletTarget& outTarget, const Json& inNode) const;
	bool readFromCMakeTarget(CMakeTarget& outTarget, const Json& inNode) const;
	bool readFromMesonTarget(MesonTarget& outTarget, const Json& inNode) const;
	bool readFromScriptTarget(ScriptBuildTarget& outTarget, const Json& inNode) const;
	bool readFromProcessTarget(ProcessBuildTarget& outTarget, const Json& inNode) const;
	bool readFromValidationTarget(ValidationBuildTarget& outTarget, const Json& inNode) const;
	bool readFromRunTargetProperties(IBuildTarget& outTarget, const Json& inNode) const;
	bool readFromCompilerSettingsCxx(SourceTarget& outTarget, const Json& inNode) const;
	bool readFromSourceTargetMetadata(SourceTarget& outTarget, const Json& inNode) const;

	bool readFromDistribution(const Json& inNode, const std::string& inFilename) const;
	bool readFromDistributionArchive(BundleArchiveTarget& outTarget, const Json& inNode) const;
	bool readFromDistributionBundle(BundleTarget& outTarget, const Json& inNode, const std::string& inFilename, const Json& inRoot) const;
	bool readFromMacosDiskImage(MacosDiskImageTarget& outTarget, const Json& inNode) const;
	bool readFromDistributionScript(ScriptDistTarget& outTarget, const Json& inNode) const;
	bool readFromDistributionProcess(ProcessDistTarget& outTarget, const Json& inNode) const;
	bool readFromDistributionValidation(ValidationDistTarget& outTarget, const Json& inNode) const;

	TriBool readFromTargetCondition(IBuildTarget& outTarget, const Json& inNode) const;
	TriBool readFromTargetCondition(IDistTarget& outTarget, const Json& inNode) const;

	bool validBuildRequested(const std::string& inFilename) const;
	std::string getValidRunTargetFromInput(const std::string& inFilename) const;
	TriBool conditionIsValid(const std::string& inTargetName, const std::string& inContent) const;
	TriBool conditionIsValid(const std::string& inContent) const;
	ConditionResult checkConditionVariable(const std::string& inTargetName, const std::string& inString, const std::string& key, const std::string& value, bool negate) const;
	ConditionResult checkConditionVariable(const std::string& inString, const std::string& key, const std::string& value, bool negate) const;

	template <typename T>
	bool valueMatchesSearchKeyPattern(T& outVariable, const Json& inNode, const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const;
	bool valueMatchesSearchKeyPattern(const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const;

	BuildState& m_state;

	PropertyConditionMatcher m_propertyConditions;

	const StringList kValidPlatforms;

	std::string m_platform;
	mutable StringList m_buildTargets;

	bool m_isWebPlatform = false;
};
}

#include "ChaletJson/ChaletJsonFile.inl"
