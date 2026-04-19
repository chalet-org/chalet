/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "ChaletJson/ChaletJsonFileCentral.hpp"

#include "ChaletJson/ChaletJsonSchema.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "State/CentralState.hpp"
#include "State/TargetMetadata.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonKeys.hpp"
#include "Json/JsonValues.hpp"

#include "State/Dependency/ArchiveDependency.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "State/Dependency/LocalDependency.hpp"
#include "State/Dependency/ScriptDependency.hpp"

namespace chalet
{
/*****************************************************************************/
bool ChaletJsonFileCentral::read(CentralState& inCentralState)
{
	auto& buildFile = inCentralState.buildFile();
	ChaletJsonFileCentral chaletJsonFileCentral(inCentralState);
	return chaletJsonFileCentral.readFrom(buildFile);
}

/*****************************************************************************/
bool ChaletJsonFileCentral::readPackages(CentralState& inCentralState, const JsonFile& inJsonFile, StringList& outTargets)
{
	auto& jRoot = inJsonFile.root;
	auto& jFilename = inJsonFile.filename();

	ChaletJsonFileCentral centralParser(inCentralState);

	constexpr bool kForPackages = true;
	if (!centralParser.readFromExternalDependencies(jRoot, jFilename, kForPackages))
		return false;

	if (!centralParser.getExternalBuildTargets(jRoot, outTargets))
		return false;

	return true;
}

/*****************************************************************************/
ChaletJsonFileCentral::ChaletJsonFileCentral(CentralState& inCentralState) :
	m_centralState(inCentralState),
	kValidPlatforms(Platform::validPlatforms())
{
	Platform::assignPlatform(m_centralState.inputs(), m_platform);
}

/*****************************************************************************/
ChaletJsonFileCentral::~ChaletJsonFileCentral() = default;

/*****************************************************************************/
bool ChaletJsonFileCentral::readFrom(JsonFile& inJsonFile)
{
	if (!validateAgainstSchema(inJsonFile))
		return false;

	auto& jRoot = inJsonFile.root;
	auto& jFilename = inJsonFile.filename();
	if (!readFromRoot(jRoot, jFilename))
		return false;

	if (!readFromVariables(jRoot, jFilename))
		return false;

	if (!readFromMetadata(jRoot))
		return false;

	if (!readFromAllowedArchitectures(jRoot, jFilename))
		return false;

	if (!readFromDefaultConfigurations(jRoot, jFilename))
		return false;

	if (!readFromConfigurations(jRoot, jFilename))
		return false;

	if (!readFromExternalDependencies(jRoot, jFilename))
		return false;

	return true;
}

/*****************************************************************************/
bool ChaletJsonFileCentral::validateAgainstSchema(const JsonFile& inJsonFile) const
{
	Json jsonSchema;

	if (m_centralState.inputs().saveSchemaToFile())
	{
		jsonSchema = ChaletJsonSchema::get(m_centralState.inputs());
		JsonFile::saveToFile(jsonSchema, "schema/chalet.schema.json");
	}

	bool buildFileChanged = m_centralState.cache.file().buildFileChanged();
	if (buildFileChanged)
	{
		if (jsonSchema.empty())
			jsonSchema = ChaletJsonSchema::get(m_centralState.inputs());

		if (!inJsonFile.validate(jsonSchema))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonFileCentral::readFromRoot(const Json& inNode, const std::string& inFilename) const
{
	if (!inNode.is_object())
	{
		Diagnostic::error("{}: Json root must be an object.", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonFileCentral::readFromMetadata(const Json& inNode) const
{
	auto metadata = std::make_shared<TargetMetadata>();
	for (const auto& [key, value] : inNode.items())
	{
		if (value.is_string())
		{
			if (String::equals(Keys::MetaName, key))
				metadata->setName(value.get<std::string>());
			else if (String::equals(Keys::MetaVersion, key))
				metadata->setVersion(value.get<std::string>());
			else if (String::equals(Keys::MetaDescription, key))
				metadata->setDescription(value.get<std::string>());
			else if (String::equals(Keys::MetaHompage, key))
				metadata->setHomepage(value.get<std::string>());
			else if (String::equals(Keys::MetaAuthor, key))
				metadata->setAuthor(value.get<std::string>());
			else if (String::equals(Keys::MetaLicense, key))
				metadata->setLicense(value.get<std::string>());
			else if (String::equals(Keys::MetaReadme, key))
				metadata->setReadme(value.get<std::string>());
		}
	}

	m_centralState.workspace.setMetadata(std::move(metadata));

	return true;
}

/*****************************************************************************/
bool ChaletJsonFileCentral::readFromVariables(const Json& inNode, const std::string& inFilename) const
{
	if (inNode.contains(Keys::Variables))
	{
		const Json& variables = inNode[Keys::Variables];
		if (variables.is_object())
		{
			auto& vars = m_centralState.tools.variables;
			for (auto& [key, value] : variables.items())
			{
				if (value.is_string())
				{
					if (!vars.contains(key))
					{
						vars.set(key, value.get<std::string>());
					}
					else
					{
						Diagnostic::warn("{}: Variable not set because it already exists: {}", inFilename, key);
					}
				}
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonFileCentral::readFromAllowedArchitectures(const Json& inNode, const std::string& inFilename) const
{
	if (inNode.contains(Keys::AllowedArchitectures))
	{
		const Json& allowedArchitectures = inNode[Keys::AllowedArchitectures];
		if (allowedArchitectures.is_array())
		{
			for (auto& archJson : allowedArchitectures)
			{
				if (archJson.is_string())
				{
					auto name = archJson.get<std::string>();
					if (name.empty())
					{
						Diagnostic::error("{}: '{}' cannot contain blank keys.", inFilename, Keys::Configurations);
						return false;
					}

					m_centralState.addAllowedArchitecture(std::move(name));
				}
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonFileCentral::readFromDefaultConfigurations(const Json& inNode, const std::string& inFilename) const
{
	bool addedDefaults = false;
	if (inNode.contains(Keys::DefaultConfigurations))
	{
		const Json& defaultConfigurations = inNode[Keys::DefaultConfigurations];
		if (defaultConfigurations.is_array())
		{
			addedDefaults = true;
			for (auto& configJson : defaultConfigurations)
			{
				if (configJson.is_string())
				{
					auto name = configJson.get<std::string>();
					if (name.empty())
					{
						Diagnostic::error("{}: '{}' cannot contain blank keys.", inFilename, Keys::Configurations);
						return false;
					}

					BuildConfiguration config;
					if (!BuildConfiguration::makeDefaultConfiguration(config, name))
					{
						Diagnostic::error("{}: Error creating the default build configuration '{}'", inFilename, name);
						return false;
					}

					m_centralState.addBuildConfiguration(name, std::move(config));
				}
			}
		}
	}

	if (!addedDefaults)
	{
		if (!m_centralState.makeDefaultBuildConfigurations())
			return false;
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonFileCentral::readFromConfigurations(const Json& inNode, const std::string& inFilename) const
{
	if (inNode.contains(Keys::Configurations))
	{
		const Json& configurations = inNode[Keys::Configurations];
		if (configurations.is_object())
		{
			for (auto& [name, configJson] : configurations.items())
			{
				if (!configJson.is_object())
				{
					Diagnostic::error("{}: configuration '{}' must be an object.", inFilename, name);
					return false;
				}

				if (name.empty())
				{
					Diagnostic::error("{}: '{}' cannot contain blank keys.", inFilename, Keys::Configurations);
					return false;
				}

				BuildConfiguration config;
				config.setName(name);

				for (const auto& [key, value] : configJson.items())
				{
					if (value.is_string())
					{
						if (String::equals("optimizationLevel", key))
							config.setOptimizationLevel(value.get<std::string>());
						else if (String::equals("sanitize", key))
							config.addSanitizeOption(value.get<std::string>());
					}
					else if (value.is_boolean())
					{
						if (String::equals("interproceduralOptimization", key))
							config.setInterproceduralOptimization(value.get<bool>());
						else if (String::equals("debugSymbols", key))
							config.setDebugSymbols(value.get<bool>());
						else if (String::equals("enableProfiling", key))
							config.setEnableProfiling(value.get<bool>());
					}
					else if (value.is_array())
					{
						if (String::equals("sanitize", key))
							config.addSanitizeOptions(value.get<StringList>());
					}
				}

				m_centralState.addBuildConfiguration(name, std::move(config));
			}
		}
	}

	return true;
}

/*****************************************************************************/
// note: just return true here for now and ignore errors in dependent build files
//
bool ChaletJsonFileCentral::getExternalBuildTargets(const Json& inNode, StringList& outTargets) const
{
	if (!inNode.contains(Keys::Targets))
		return true; // We don't care

	const Json& targets = inNode[Keys::Targets];
	if (!targets.is_object() || targets.empty())
		return true;

	for (auto& [name, targetJson] : targets.items())
	{
		if (String::equals(Values::All, name))
			continue;

		if (!targetJson.is_object())
			continue;

		StringList externalTargets{
			"chaletProject",
			"cmakeProject",
			"mesonProject",
		};

		if (std::string val; json::assign(val, targetJson, "kind"))
		{
			if (String::equals(externalTargets, val))
			{
				List::addIfDoesNotExist(outTargets, name);
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonFileCentral::readFromExternalDependencies(const Json& inNode, const std::string& inFilename, const bool inForPackages) const
{
	// don't care if there aren't any dependencies
	if (!inNode.contains(Keys::ExternalDependencies))
		return true;

	const Json& externalDependencies = inNode[Keys::ExternalDependencies];
	if (!externalDependencies.is_object() || externalDependencies.empty())
	{
		Diagnostic::error("{}: '{}' must contain at least one external dependency.", inFilename, Keys::ExternalDependencies);
		return false;
	}

	for (auto& [name, dependencyJson] : externalDependencies.items())
	{
		if (!dependencyJson.is_object())
		{
			Diagnostic::error("{}: external dependency '{}' must be an object.", inFilename, name);
			return false;
		}

		ExternalDependencyType type = ExternalDependencyType::Local;
		if (std::string val; json::assign(val, dependencyJson, "kind"))
		{
			if (String::equals("git", val))
			{
				type = ExternalDependencyType::Git;
			}
			else if (String::equals("local", val))
			{
				type = ExternalDependencyType::Local;
			}
			else if (String::equals("archive", val))
			{
				type = ExternalDependencyType::Archive;
			}
			else if (String::equals("script", val))
			{
				type = ExternalDependencyType::Script;
			}
			else
			{
				Diagnostic::error("{}: Found unrecognized external dependency kind of '{}'", inFilename, val);
				return false;
			}
		}
		else
		{
			Diagnostic::error("{}: Found unrecognized external dependency of '{}'", inFilename, name);
			return false;
		}

		ExternalDependency dependency = IExternalDependency::make(type, m_centralState);
		dependency->setName(name);

		TriBool conditionResult = readFromCondition(dependencyJson);
		if (conditionResult == TriBool::Unset)
			return false;

		if (conditionResult == TriBool::False)
			continue; // Skip dependency

		if (dependency->isGit())
		{
			if (!inForPackages && !readFromGitDependency(static_cast<GitDependency&>(*dependency), dependencyJson))
				return false;
		}
		else if (dependency->isLocal())
		{
			if (!readFromLocalDependency(static_cast<LocalDependency&>(*dependency), dependencyJson, inFilename))
			{
				Diagnostic::error("{}: Error parsing the '{}' dependency of type 'local'.", inFilename, name);
				return false;
			}
		}
		else if (dependency->isArchive())
		{
			if (!inForPackages && !readFromArchiveDependency(static_cast<ArchiveDependency&>(*dependency), dependencyJson, inFilename))
			{
				Diagnostic::error("{}: Error parsing the '{}' dependency of type 'archive'.", inFilename, name);
				return false;
			}
		}
		else if (dependency->isScript())
		{
			// A script could be only for a specific platform
			if (!inForPackages && !readFromScriptDependency(static_cast<ScriptDependency&>(*dependency), dependencyJson, inFilename))
				return false;
		}
		else
		{
			Diagnostic::error("{}: Unknown external dependency: {}", inFilename, name);
			return false;
		}

		m_centralState.externalDependencies.emplace_back(std::move(dependency));
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonFileCentral::readFromGitDependency(GitDependency& outDependency, const Json& inNode) const
{
	for (const auto& [key, value] : inNode.items())
	{
		if (value.is_string())
		{
			if (String::equals("repository", key))
				outDependency.setRepository(value.get<std::string>());
			else if (String::equals("branch", key))
				outDependency.setBranch(value.get<std::string>());
			else if (String::equals("tag", key))
				outDependency.setTag(value.get<std::string>());
			else if (String::equals("commit", key))
				outDependency.setCommit(value.get<std::string>());
		}
		else if (value.is_boolean())
		{
			if (String::equals("submodules", key))
				outDependency.setSubmodules(value.get<bool>());
		}
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonFileCentral::readFromLocalDependency(LocalDependency& outDependency, const Json& inNode, const std::string& inFilename) const
{
	for (const auto& [key, value] : inNode.items())
	{
		if (value.is_string())
		{
			if (String::equals("path", key))
				outDependency.setPath(value.get<std::string>());
		}
	}

	bool hasPath = !outDependency.path().empty();
	if (!hasPath)
	{
		Diagnostic::error("{}: 'path' is required for local dependencies.", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonFileCentral::readFromArchiveDependency(ArchiveDependency& outDependency, const Json& inNode, const std::string& inFilename) const
{
	for (const auto& [key, value] : inNode.items())
	{
		if (value.is_string())
		{
			if (String::equals("url", key))
				outDependency.setUrl(value.get<std::string>());
			else if (String::equals("subdirectory", key))
				outDependency.setSubdirectory(value.get<std::string>());
		}
	}

	bool hasPath = !outDependency.url().empty();
	if (!hasPath)
	{
		Diagnostic::error("{}: 'url' is required for archive dependencies.", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonFileCentral::readFromScriptDependency(ScriptDependency& outDependency, const Json& inNode, const std::string& inFilename) const
{
	bool valid = false;
	for (const auto& [key, value] : inNode.items())
	{
		if (value.is_string())
		{
			if (String::equals("file", key))
			{
				outDependency.setFile(value.get<std::string>());
				valid = true;
			}
			else if (String::equals("arguments", key))
				outDependency.addArgument(value.get<std::string>());
			else if (String::equals("workingDirectory", key))
				outDependency.setWorkingDirectory(value.get<std::string>());
		}
		else if (value.is_array())
		{
			if (String::equals("arguments", key))
				outDependency.addArguments(value.get<StringList>());
		}
	}

	if (!valid)
	{
		Diagnostic::error("{}: 'file' is required for script dependencies.", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
TriBool ChaletJsonFileCentral::readFromCondition(const Json& inNode) const
{
	if (std::string val; json::assign(val, inNode, "condition"))
	{
		if (!m_propertyConditions.match(val, [this, &val](const std::string& key, const std::string& value, bool negate) {
				auto res = checkConditionVariable(val, key, value, negate);
				return res == ConditionResult::Pass;
			}))
		{
			if (m_propertyConditions.lastOp == ConditionOp::InvalidOr)
			{
				Diagnostic::error("Syntax for AND '+', OR '|' are mutually exclusive. Both found in: {}", val);
				return TriBool::Unset;
			}

			return TriBool::False;
		}
	}

	return TriBool::True;
}

/*****************************************************************************/
ConditionResult ChaletJsonFileCentral::checkConditionVariable(const std::string& inString, const std::string& key, const std::string& value, bool negate) const
{
	// LOG("  ", key, value, negate);

	if (key.empty())
	{
		if (String::equals(kValidPlatforms, value))
		{
			bool isPlatform = String::equals(m_platform, value);
			if (negate)
			{
				if (isPlatform)
					return ConditionResult::Fail;
			}
			else
			{
				if (!isPlatform)
					return ConditionResult::Fail;
			}
		}
		else
		{
			Diagnostic::error("Invalid condition '{}' found in: {}", value, inString);
			return ConditionResult::Invalid;
		}
	}
	else if (String::equals("platform", key))
	{
		if (String::equals(kValidPlatforms, value))
		{
			bool isPlatform = String::equals(m_platform, value);
			if (negate)
			{
				if (isPlatform)
					return ConditionResult::Fail;
			}
			else
			{
				if (!isPlatform)
					return ConditionResult::Fail;
			}
		}
		else
		{
			Diagnostic::error("Invalid platform '{}' found in: {}", value, inString);
			return ConditionResult::Invalid;
		}
	}
	else if (String::equals("env", key))
	{
		auto res = Environment::get(value.c_str());
		if (negate)
		{
			if (res != nullptr)
				return ConditionResult::Fail;
		}
		else
		{
			if (res == nullptr)
				return ConditionResult::Fail;
		}
	}
	else
	{
		Diagnostic::error("Invalid condition property '{}' found in: {}", key, inString);
		return ConditionResult::Invalid;
	}

	return ConditionResult::Pass;
}

}
