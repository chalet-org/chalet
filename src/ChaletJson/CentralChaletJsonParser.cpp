/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "ChaletJson/CentralChaletJsonParser.hpp"

#include "ChaletJson/ChaletJsonSchema.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/CentralState.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "State/TargetMetadata.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
CentralChaletJsonParser::CentralChaletJsonParser(CentralState& inCentralState) :
	m_centralState(inCentralState),
	m_chaletJson(inCentralState.chaletJson()),
	m_notPlatforms(Platform::notPlatforms()),
	m_platform(Platform::platform())
{
}

/*****************************************************************************/
CentralChaletJsonParser::~CentralChaletJsonParser() = default;

/*****************************************************************************/
bool CentralChaletJsonParser::serialize() const
{
	if (!validateAgainstSchema())
		return false;

	const Json& jRoot = m_chaletJson.json;
	if (!serializeRequiredFromJsonRoot(jRoot))
		return false;

	return true;
}

/*****************************************************************************/
bool CentralChaletJsonParser::validateAgainstSchema() const
{
	ChaletJsonSchema schemaBuilder;
	Json jsonSchema = schemaBuilder.get();

	if (m_centralState.inputs().saveSchemaToFile())
	{
		JsonFile::saveToFile(jsonSchema, "schema/chalet.schema.json");
	}

	// TODO: schema versioning
	if (!m_chaletJson.validate(std::move(jsonSchema)))
		return false;

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
		const Json& variables = inNode.at(Keys::Variables);
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
bool CentralChaletJsonParser::parseDefaultConfigurations(const Json& inNode) const
{
	bool addedDefaults = false;
	if (inNode.contains(Keys::DefaultConfigurations))
	{
		const Json& defaultConfigurations = inNode.at(Keys::DefaultConfigurations);
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
		const Json& configurations = inNode.at(Keys::Configurations);
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
						StringList val;
						if (String::equals("sanitize", key))
							config.addSanitizeOptions(std::move(val));
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

	const Json& externalDependencies = inNode.at(Keys::ExternalDependencies);
	if (!externalDependencies.is_object() || externalDependencies.size() == 0)
	{
		Diagnostic::error("{}: '{}' must contain at least one external dependency.", m_chaletJson.filename(), Keys::ExternalDependencies);
		return false;
	}

	BuildDependencyType type = BuildDependencyType::Git;
	for (auto& [name, dependencyJson] : externalDependencies.items())
	{
		auto dependency = IBuildDependency::make(type, m_centralState);
		dependency->setName(name);

		if (!parseGitDependency(static_cast<GitDependency&>(*dependency), dependencyJson))
			return false;

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

	bool hasRepository = !outDependency.repository().empty();
	bool hasCommit = !outDependency.commit().empty();
	bool hasTag = !outDependency.tag().empty();

	if (!hasRepository)
	{
		Diagnostic::error("{}: 'repository' is required for all  external dependencies.", m_chaletJson.filename());
		return false;
	}

	if (hasCommit && hasTag)
	{
		if (!outDependency.tag().empty())
		{
			Diagnostic::error("{}: Dependencies cannot contain both 'tag' and 'commit'. Found in '{}'", m_chaletJson.filename(), outDependency.repository());
			return false;
		}
	}

	return true;
}

}
