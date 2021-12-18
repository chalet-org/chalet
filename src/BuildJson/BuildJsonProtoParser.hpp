/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_JSON_PROTO_PARSER_HPP
#define CHALET_BUILD_JSON_PROTO_PARSER_HPP

#include "Core/Platform.hpp"
#include "Libraries/Json.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonNodeReadStatus.hpp"

namespace chalet
{
struct CommandLineInputs;
struct StatePrototype;
struct JsonFile;
struct GitDependency;
struct IDistTarget;
struct BundleTarget;
struct ScriptDistTarget;
struct BundleArchiveTarget;
struct MacosDiskImageTarget;
struct WindowsNullsoftInstallerTarget;

struct BuildJsonProtoParser
{
	explicit BuildJsonProtoParser(const CommandLineInputs& inInputs, StatePrototype& inPrototype);
	CHALET_DISALLOW_COPY_MOVE(BuildJsonProtoParser);
	~BuildJsonProtoParser();

	bool serialize() const;

private:
	bool validateAgainstSchema() const;
	bool serializeRequiredFromJsonRoot(const Json& inNode) const;

	bool parseRoot(const Json& inNode) const;

	bool parseDefaultConfigurations(const Json& inNode) const;
	bool parseConfigurations(const Json& inNode) const;

	bool parseDistribution(const Json& inNode) const;
	bool parseDistributionScript(ScriptDistTarget& outTarget, const Json& inNode) const;
	bool parseDistributionArchive(BundleArchiveTarget& outTarget, const Json& inNode) const;
	bool parseDistributionBundle(BundleTarget& outTarget, const Json& inNode) const;
	bool parseMacosDiskImage(MacosDiskImageTarget& outTarget, const Json& inNode) const;
	bool parseWindowsNullsoftInstaller(WindowsNullsoftInstallerTarget& outTarget, const Json& inNode) const;

	bool parseExternalDependencies(const Json& inNode) const;
	bool parseGitDependency(GitDependency& outDependency, const Json& inNode) const;
	bool parseTargetCondition(IDistTarget& outTarget, const Json& inNode) const;

	//
	template <typename T>
	bool valueMatchesSearchKeyPattern(T& outVariable, const Json& inNode, const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const;

	bool conditionIsValid(const std::string& inContent) const;

	const CommandLineInputs& m_inputs;
	StatePrototype& m_prototype;
	JsonFile& m_chaletJson;
	const std::string& m_filename;

	StringList m_notPlatforms;
	std::string m_platform;

	const std::string kKeyDistribution = "distribution";
	const std::string kKeyConfigurations = "configurations";
	const std::string kKeyDefaultConfigurations = "defaultConfigurations";
	const std::string kKeyExternalDependencies = "externalDependencies";
};
}

#include "BuildJson/BuildJsonProtoParser.inl"

#endif // CHALET_BUILD_JSON_PROTO_PARSER_HPP
