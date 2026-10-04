/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "ChaletJson/PropertyConditionMatcher.hpp"
#include "Libraries/Json.hpp"
#include "Platform/Platform.hpp"
#include "Process/Environment.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/TriBool.hpp"
#include "Json/IJsonFileReader.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonNodeReadStatus.hpp"

namespace chalet
{
struct CentralState;
struct JsonFile;

struct ArchiveDependency;
struct GitDependency;
struct LocalDependency;
struct ScriptDependency;

struct ChaletJsonFileCentral final : public IJsonFileReader
{
	static bool read(CentralState& inCentralState);
	static bool readPackages(CentralState& inCentralState, const JsonFile& inJsonFile, StringList& outTargets);

private:
	explicit ChaletJsonFileCentral(CentralState& inCentralState);
	CHALET_DISALLOW_COPY_MOVE(ChaletJsonFileCentral);
	~ChaletJsonFileCentral();

	virtual bool readFrom(JsonFile& inJsonFile) final;

	bool getExternalBuildTargets(const Json& inNode, StringList& outTargets) const;

	bool validateAgainstSchema(const JsonFile& inJsonFile) const;

	bool readFromRoot(const Json& inNode, const std::string& inFilename) const;
	bool readFromMetadata(const Json& inNode) const;

	bool readFromVariables(const Json& inNode, const std::string& inFilename) const;
	bool readFromAllowedArchitectures(const Json& inNode, const std::string& inFilename) const;
	bool readFromDefaultConfigurations(const Json& inNode, const std::string& inFilename) const;
	bool readFromConfigurations(const Json& inNode, const std::string& inFilename) const;
	bool readFromExternalDependencies(const Json& inNode, const std::string& inFilename, const bool inForPackages = false) const;

	bool readFromGitDependency(GitDependency& outDependency, const Json& inNode) const;
	bool readFromLocalDependency(LocalDependency& outDependency, const Json& inNode, const std::string& inFilename) const;
	bool readFromArchiveDependency(ArchiveDependency& outDependency, const Json& inNode, const std::string& inFilename) const;
	bool readFromScriptDependency(ScriptDependency& outDependency, const Json& inNode, const std::string& inFilename) const;

	TriBool readFromCondition(const Json& inNode) const;
	ConditionResult checkConditionVariable(const std::string& inString, const std::string& key, const std::string& value, bool negate) const;

	CentralState& m_centralState;

	PropertyConditionMatcher m_propertyConditions;

	const StringList kValidPlatforms;

	std::string m_platform;
};
}
