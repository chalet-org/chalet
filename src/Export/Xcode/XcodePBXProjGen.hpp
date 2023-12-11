/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/SourceType.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/Uuid.hpp"

namespace chalet
{
class BuildState;

struct XcodePBXProjGen
{
	enum class ListType
	{
		Project,
		NativeProject,
		AggregateTarget,
	};

	explicit XcodePBXProjGen(std::vector<Unique<BuildState>>& inStates, const std::string& inAllBuildName);

	bool saveToFile(const std::string& inFilename);

private:
	std::string getHashWithLabel(const std::string& inValue) const;
	std::string getHashWithLabel(const Uuid& inHash, const std::string& inLabel) const;
	Uuid getTargetHash(const std::string& inTarget) const;
	Uuid getDistTargetHash(const std::string& inTarget) const;
	Uuid getConfigurationHash(const std::string& inConfig) const;
	Uuid getTargetConfigurationHash(const std::string& inConfig, const std::string& inTarget, const bool inDist = false) const;
	std::string getTargetHashWithLabel(const std::string& inTarget, const bool inDist = false) const;
	std::string getSectionKeyForTarget(const std::string& inKey, const std::string& inTarget) const;
	std::string getBuildConfigurationListLabel(const std::string& inName, const ListType inType) const;
	std::string getProjectName() const;

	Json getHashedJsonValue(const std::string& inValue) const;
	Json getHashedJsonValue(const Uuid& inHash, const std::string& inLabel) const;

	std::string getBoolString(const bool inValue) const;
	std::string getXcodeFileType(const SourceType inType) const;
	std::string getXcodeFileType(const SourceKind inKind) const;
	std::string getXcodeFileTypeFromHeader(const std::string& inFile) const;
	std::string getXcodeFileTypeFromFile(const std::string& inFile) const;
	std::string getMachOType(const SourceTarget& inTarget) const;
	std::string getNativeProductType(const SourceKind inKind) const;
	std::string getSourceWithSuffix(const std::string& inFile, const std::string& inSuffix) const;

	Json getProductBuildSettings(const BuildState& inState) const;
	Json getBuildSettings(BuildState& inState, const SourceTarget& inTarget) const;
	Json getGenericBuildSettings(BuildState& inState, const IBuildTarget& inTarget) const;
	Json getExcludedBuildSettings(BuildState& inState, const std::string& inTargetName) const;
	Json getAppBundleBuildSettings(BuildState& inState, const BundleTarget& inTarget) const;

	StringList getCompilerOptions(const BuildState& inState, const SourceTarget& inTarget) const;
	StringList getLinkerOptions(const BuildState& inState, const SourceTarget& inTarget) const;

	std::vector<Unique<BuildState>>& m_states;
	const std::string& m_allBuildName;

	Uuid m_projectUUID;

	mutable Json m_infoPlistJson;

	StringList m_appBuildTargets;

	std::string m_exportPath;
	std::string m_xcodeNamespaceGuid;
	std::string m_projectGuid;

	mutable std::unordered_map<std::string, bool> m_generatedBundleFiles;
};
}
