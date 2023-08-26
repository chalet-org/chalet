/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_XCODE_PBXPROJ_GEN_HPP
#define CHALET_XCODE_PBXPROJ_GEN_HPP

#include "Libraries/Json.hpp"
#include "State/SourceType.hpp"
#include "Utility/Uuid.hpp"

namespace chalet
{
class BuildState;

struct XcodePBXProjGen
{
	explicit XcodePBXProjGen(const std::vector<Unique<BuildState>>& inStates);

	bool saveToFile(const std::string& inFilename);

private:
	std::string getHashWithLabel(const std::string& inValue) const;
	std::string getHashWithLabel(const Uuid& inHash, const std::string& inLabel) const;
	std::string getTargetHashWithLabel(const std::string& inTarget) const;

	Json getHashedJsonValue(const std::string& inValue) const;
	Json getHashedJsonValue(const Uuid& inHash, const std::string& inLabel) const;

	std::string getBuildConfigurationListLabel(const std::string& inName, const bool inNativeProject = true) const;

	Json getBuildSettings(const BuildState& inState) const;
	Json getProductBuildSettings(const BuildState& inState) const;
	std::string getBoolString(const bool inValue) const;
	std::string getProductBundleIdentifier(const std::string& inWorkspaceName) const;
	std::string getLastKnownFileType(const SourceType inType) const;

	std::string generateFromJson(const Json& inJson) const;
	std::string getNodeAsPListFormat(const Json& inJson, const size_t indent = 1) const;
	std::string getNodeAsPListString(const Json& inJson) const;

	const std::vector<Unique<BuildState>>& m_states;

	Uuid m_projectUUID;

	std::string m_xcodeNamespaceGuid;
	std::string m_projectGuid;
};
}

#endif // CHALET_XCODE_PBXPROJ_GEN_HPP
