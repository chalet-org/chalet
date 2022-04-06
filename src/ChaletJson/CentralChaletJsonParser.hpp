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
struct IDistTarget;
struct BundleTarget;
struct ScriptDistTarget;
struct ProcessDistTarget;
struct BundleArchiveTarget;
struct MacosDiskImageTarget;
struct WindowsNullsoftInstallerTarget;

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

	bool parseDefaultConfigurations(const Json& inNode) const;
	bool parseConfigurations(const Json& inNode) const;

	bool parseDistribution(const Json& inNode) const;
	bool parseDistributionScript(ScriptDistTarget& outTarget, const Json& inNode) const;
	bool parseDistributionProcess(ProcessDistTarget& outTarget, const Json& inNode) const;
	bool parseDistributionArchive(BundleArchiveTarget& outTarget, const Json& inNode) const;
	bool parseDistributionBundle(BundleTarget& outTarget, const Json& inNode, const Json& inRoot) const;
	bool parseMacosDiskImage(MacosDiskImageTarget& outTarget, const Json& inNode) const;
	bool parseWindowsNullsoftInstaller(WindowsNullsoftInstallerTarget& outTarget, const Json& inNode) const;

	bool parseExternalDependencies(const Json& inNode) const;
	bool parseGitDependency(GitDependency& outDependency, const Json& inNode) const;
	bool parseTargetCondition(IDistTarget& outTarget, const Json& inNode) const;

	//
	template <typename T>
	bool valueMatchesSearchKeyPattern(T& outVariable, const Json& inNode, const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const;

	bool conditionIsValid(const std::string& inContent) const;

	CentralState& m_centralState;
	JsonFile& m_chaletJson;

	StringList m_notPlatforms;
	std::string m_platform;
};
}

#include "ChaletJson/CentralChaletJsonParser.inl"

#endif // CHALET_CENTRAL_CHALET_JSON_PARSER_HPP
