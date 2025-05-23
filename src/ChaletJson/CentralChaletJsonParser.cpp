/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "ChaletJson/CentralChaletJsonParser.hpp"

#include "ChaletJson/ChaletJsonSchema.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "State/CentralState.hpp"
#include "State/TargetMetadata.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonKeys.hpp"

#include "State/Dependency/ArchiveDependency.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "State/Dependency/LocalDependency.hpp"
#include "State/Dependency/ScriptDependency.hpp"

namespace chalet
{
/*****************************************************************************/
CentralChaletJsonParser::CentralChaletJsonParser(CentralState& inCentralState) :
	m_centralState(inCentralState),
	m_chaletJson(inCentralState.chaletJson()),
	kValidPlatforms(Platform::validPlatforms())
{
	Platform::assignPlatform(m_centralState.inputs(), m_platform, m_notPlatforms);
}

/*****************************************************************************/
CentralChaletJsonParser::~CentralChaletJsonParser() = default;

/*****************************************************************************/
bool CentralChaletJsonParser::serialize() const
{
	if (!validateAgainstSchema())
		return false;

	const Json& jRoot = m_chaletJson.root;
	if (!serializeRequiredFromJsonRoot(jRoot))
		return false;

	return true;
}

/*****************************************************************************/
bool CentralChaletJsonParser::validateAgainstSchema() const
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

		if (!m_chaletJson.validate(jsonSchema))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool CentralChaletJsonParser::serializeRequiredFromJsonRoot(const Json& inNode) const
{
	if (!inNode.is_object())
		return false;

	if (!parseRoot(inNode))
		return false;

	if (!parseVariables(inNode))
		return false;

	if (!parseMetadata(inNode))
		return false;

	if (!parseAllowedArchitectures(inNode))
		return false;

	if (!parseDefaultConfigurations(inNode))
		return false;

	if (!parseConfigurations(inNode))
		return false;

	if (!parseExternalDependencies(inNode))
		return false;

	return true;
}

/*****************************************************************************/
bool CentralChaletJsonParser::parseRoot(const Json& inNode) const
{
	if (!inNode.is_object())
	{
		Diagnostic::error("{}: Json root must be an object.", m_chaletJson.filename());
		return false;
	}

	return true;
}

/*****************************************************************************/
bool CentralChaletJsonParser::parseMetadata(const Json& inNode) const
{
	auto metadata = std::make_shared<TargetMetadata>();
	for (const auto& [key, value] : inNode.items())
	{
		if (value.is_string())
		{
			if (String::equals("name", key))
				metadata->setName(value.get<std::string>());
			else if (String::equals("version", key))
				metadata->setVersion(value.get<std::string>());
			else if (String::equals("description", key))
				metadata->setDescription(value.get<std::string>());
			else if (String::equals("homepage", key))
				metadata->setHomepage(value.get<std::string>());
			else if (String::equals("author", key))
				metadata->setAuthor(value.get<std::string>());
			else if (String::equals("license", key))
				metadata->setLicense(value.get<std::string>());
			else if (String::equals("readme", key))
				metadata->setReadme(value.get<std::string>());
		}
	}

	m_centralState.workspace.setMetadata(std::move(metadata));

	return true;
}

/*****************************************************************************/
bool CentralChaletJsonParser::parseVariables(const Json& inNode) const
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
						Diagnostic::warn("Variable not set because it already exists: {}", key);
					}
				}
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool CentralChaletJsonParser::parseAllowedArchitectures(const Json& inNode) const
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
						Diagnostic::error("{}: '{}' cannot contain blank keys.", m_chaletJson.filename(), Keys::Configurations);
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
bool CentralChaletJsonParser::parseDefaultConfigurations(const Json& inNode) const
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
						Diagnostic::error("{}: '{}' cannot contain blank keys.", m_chaletJson.filename(), Keys::Configurations);
						return false;
					}

					BuildConfiguration config;
					if (!BuildConfiguration::makeDefaultConfiguration(config, name))
					{
						Diagnostic::error("{}: Error creating the default build configuration '{}'", m_chaletJson.filename(), name);
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
bool CentralChaletJsonParser::parseConfigurations(const Json& inNode) const
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
					Diagnostic::error("{}: configuration '{}' must be an object.", m_chaletJson.filename(), name);
					return false;
				}

				if (name.empty())
				{
					Diagnostic::error("{}: '{}' cannot contain blank keys.", m_chaletJson.filename(), Keys::Configurations);
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
bool CentralChaletJsonParser::parseExternalDependencies(const Json& inNode) const
{
	// don't care if there aren't any dependencies
	if (!inNode.contains(Keys::ExternalDependencies))
		return true;

	const Json& externalDependencies = inNode[Keys::ExternalDependencies];
	if (!externalDependencies.is_object() || externalDependencies.empty())
	{
		Diagnostic::error("{}: '{}' must contain at least one external dependency.", m_chaletJson.filename(), Keys::ExternalDependencies);
		return false;
	}

	for (auto& [name, dependencyJson] : externalDependencies.items())
	{
		if (!dependencyJson.is_object())
		{
			Diagnostic::error("{}: external dependency '{}' must be an object.", m_chaletJson.filename(), name);
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
				Diagnostic::error("{}: Found unrecognized external dependency kind of '{}'", m_chaletJson.filename(), val);
				return false;
			}
		}
		else
		{
			Diagnostic::error("{}: Found unrecognized external dependency of '{}'", m_chaletJson.filename(), name);
			return false;
		}

		ExternalDependency dependency = IExternalDependency::make(type, m_centralState);
		dependency->setName(name);

		auto conditionResult = parseDependencyCondition(dependencyJson);
		if (!conditionResult.has_value())
			return false;

		if (!(*conditionResult))
			continue; // true to skip project

		if (dependency->isGit())
		{
			if (!parseGitDependency(static_cast<GitDependency&>(*dependency), dependencyJson))
				return false;
		}
		else if (dependency->isLocal())
		{
			if (!parseLocalDependency(static_cast<LocalDependency&>(*dependency), dependencyJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' dependency of type 'local'.", m_chaletJson.filename(), name);
				return false;
			}
		}
		else if (dependency->isArchive())
		{
			if (!parseArchiveDependency(static_cast<ArchiveDependency&>(*dependency), dependencyJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' dependency of type 'archive'.", m_chaletJson.filename(), name);
				return false;
			}
		}
		else if (dependency->isScript())
		{
			// A script could be only for a specific platform
			if (!parseScriptDependency(static_cast<ScriptDependency&>(*dependency), dependencyJson))
				return false;
		}
		else
		{
			Diagnostic::error("{}: Unknown external dependency: {}", m_chaletJson.filename(), name);
			return false;
		}

		m_centralState.externalDependencies.emplace_back(std::move(dependency));
	}

	return true;
}

/*****************************************************************************/
bool CentralChaletJsonParser::parseGitDependency(GitDependency& outDependency, const Json& inNode) const
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
bool CentralChaletJsonParser::parseLocalDependency(LocalDependency& outDependency, const Json& inNode) const
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
		Diagnostic::error("{}: 'path' is required for local dependencies.", m_chaletJson.filename());
		return false;
	}

	return true;
}

/*****************************************************************************/
bool CentralChaletJsonParser::parseArchiveDependency(ArchiveDependency& outDependency, const Json& inNode) const
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
		Diagnostic::error("{}: 'url' is required for archive dependencies.", m_chaletJson.filename());
		return false;
	}

	return true;
}

/*****************************************************************************/
bool CentralChaletJsonParser::parseScriptDependency(ScriptDependency& outDependency, const Json& inNode) const
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
		Diagnostic::error("{}: 'file' is required for script dependencies.", m_chaletJson.filename());
		return false;
	}

	return true;
}

/*****************************************************************************/
std::optional<bool> CentralChaletJsonParser::parseDependencyCondition(const Json& inNode) const
{
	if (std::string val; json::assign(val, inNode, "condition"))
	{
		auto res = conditionIsValid(val);
		if (!res.has_value())
			return std::nullopt;

		return *res;
	}

	return true;
}

/*****************************************************************************/
std::optional<bool> CentralChaletJsonParser::conditionIsValid(const std::string& inContent) const
{
	if (!m_adapter.matchConditionVariables(inContent, [this, &inContent](const std::string& key, const std::string& value, bool negate) {
			auto res = checkConditionVariable(inContent, key, value, negate);
			return res == ConditionResult::Pass;
		}))
	{
		if (m_adapter.lastOp == ConditionOp::InvalidOr)
		{
			Diagnostic::error("Syntax for AND '+', OR '|' are mutually exclusive. Both found in: {}", inContent);
			return std::nullopt;
		}

		return false;
	}

	return true;
}

/*****************************************************************************/
ConditionResult CentralChaletJsonParser::checkConditionVariable(const std::string& inString, const std::string& key, const std::string& value, bool negate) const
{
	// LOG("  ", key, value, negate);

	if (key.empty())
	{
		if (String::equals(kValidPlatforms, value))
		{
			if (negate)
			{
				if (String::equals(value, m_platform))
					return ConditionResult::Fail;
			}
			else
			{
				if (List::contains(m_notPlatforms, value))
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
		if (!String::equals(kValidPlatforms, value))
		{
			Diagnostic::error("Invalid platform '{}' found in: {}", value, inString);
			return ConditionResult::Invalid;
		}

		if (negate)
		{
			if (String::equals(value, m_platform))
				return ConditionResult::Fail;
		}
		else
		{
			if (List::contains(m_notPlatforms, value))
				return ConditionResult::Fail;
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
