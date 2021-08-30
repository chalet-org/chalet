/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/BuildJsonProtoParser.hpp"

#include "BuildJson/SchemaBuildJson.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/ScriptDistTarget.hpp"
#include "State/StatePrototype.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
BuildJsonProtoParser::BuildJsonProtoParser(const CommandLineInputs& inInputs, StatePrototype& inPrototype) :
	m_inputs(inInputs),
	m_prototype(inPrototype),
	m_buildJson(inPrototype.jsonFile()),
	m_filename(inPrototype.filename())
{
}

/*****************************************************************************/
BuildJsonProtoParser::~BuildJsonProtoParser() = default;

/*****************************************************************************/
bool BuildJsonProtoParser::serialize()
{
	if (!validateAgainstSchema())
		return false;

	const Json& jRoot = m_buildJson.json;
	if (!serializeRequiredFromJsonRoot(jRoot))
		return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonProtoParser::serializeDependenciesOnly()
{
	if (!validateAgainstSchema())
		return false;

	const Json& jRoot = m_buildJson.json;

	if (!jRoot.is_object())
		return false;

	if (!parseRoot(jRoot))
		return false;

	if (!parseExternalDependencies(jRoot))
		return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonProtoParser::validateAgainstSchema()
{
	SchemaBuildJson schemaBuilder;
	Json buildJsonSchema = schemaBuilder.get();

	if (m_inputs.saveSchemaToFile())
	{
		JsonFile::saveToFile(buildJsonSchema, "schema/chalet.schema.json");
	}

	// TODO: schema versioning
	if (!m_buildJson.validate(std::move(buildJsonSchema)))
	{
		Diagnostic::error("{}: There was an error validating the file against its schema.", m_filename);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonProtoParser::serializeRequiredFromJsonRoot(const Json& inNode)
{
	if (!inNode.is_object())
		return false;

	if (!parseRoot(inNode))
		return false;

	if (!parseConfiguration(inNode))
		return false;

	if (!parseDistribution(inNode))
		return false;

	if (!parseExternalDependencies(inNode))
		return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonProtoParser::parseRoot(const Json& inNode)
{
	if (!inNode.is_object())
	{
		Diagnostic::error("{}: Json root must be an object.", m_filename);
		return false;
	}

	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "workspace"))
		m_prototype.environment.setWorkspace(std::move(val));

	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "version"))
		m_prototype.environment.setVersion(std::move(val));

	if (StringList list; assignStringListFromConfig(list, inNode, "searchPaths"))
		m_prototype.environment.addSearchPaths(std::move(list));

	return true;
}

/*****************************************************************************/
bool BuildJsonProtoParser::parseConfiguration(const Json& inNode)
{
	if (!inNode.contains(kKeyConfigurations))
	{
		return m_prototype.makeDefaultBuildConfigurations();
	}

	const Json& configurations = inNode.at(kKeyConfigurations);
	if (configurations.is_object())
	{
		for (auto& [name, configJson] : configurations.items())
		{
			if (!configJson.is_object())
			{
				Diagnostic::error("{}: configuration '{}' must be an object.", m_filename, name);
				return false;
			}

			if (name.empty())
			{
				Diagnostic::error("{}: '{}' cannot contain blank keys.", m_filename, kKeyConfigurations);
				return false;
			}

			BuildConfiguration config;
			config.setName(name);

			if (std::string val; m_buildJson.assignStringAndValidate(val, configJson, "optimizationLevel"))
				config.setOptimizationLevel(std::move(val));

			if (bool val = false; m_buildJson.assignFromKey(val, configJson, "linkTimeOptimization"))
				config.setLinkTimeOptimization(val);

			if (bool val = false; m_buildJson.assignFromKey(val, configJson, "stripSymbols"))
				config.setStripSymbols(val);

			if (bool val = false; m_buildJson.assignFromKey(val, configJson, "debugSymbols"))
				config.setDebugSymbols(val);

			if (bool val = false; m_buildJson.assignFromKey(val, configJson, "enableProfiling"))
				config.setEnableProfiling(val);

			if (m_prototype.releaseConfiguration().empty())
			{
				if (!config.isDebuggable())
				{
					m_prototype.setReleaseConfiguration(config.name());
				}
			}

			m_prototype.addBuildConfiguration(std::move(name), std::move(config));
		}
	}
	else if (configurations.is_array())
	{
		for (auto& configJson : configurations)
		{
			if (configJson.is_string())
			{
				auto name = configJson.get<std::string>();
				if (name.empty())
				{
					Diagnostic::error("{}: '{}' cannot contain blank keys.", m_filename, kKeyConfigurations);
					return false;
				}

				BuildConfiguration config;
				if (!m_prototype.getDefaultBuildConfiguration(config, name))
				{
					Diagnostic::error("{}: Error creating the default build configuration '{}'", m_filename, name);
					return false;
				}

				if (m_prototype.releaseConfiguration().empty())
				{
					if (!config.isDebuggable())
					{
						m_prototype.setReleaseConfiguration(config.name());
					}
				}

				m_prototype.addBuildConfiguration(std::move(name), std::move(config));
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonProtoParser::parseDistribution(const Json& inNode)
{
	if (!inNode.contains(kKeyDistribution))
		return true;

	const Json& distributionJson = inNode.at(kKeyDistribution);
	if (!distributionJson.is_object() || distributionJson.size() == 0)
	{
		Diagnostic::error("{}: '{}' must contain at least one bundle or script.", m_filename, kKeyDistribution);
		return false;
	}

	for (auto& [name, targetJson] : distributionJson.items())
	{
		if (!targetJson.is_object())
		{
			Diagnostic::error("{}: distribution bundle '{}' must be an object.", m_filename, name);
			return false;
		}

		DistTargetType type = DistTargetType::DistributionBundle;
		if (m_buildJson.containsKeyThatStartsWith(targetJson, "script"))
		{
			type = DistTargetType::Script;
		}

		DistTarget target = IDistTarget::make(type);
		target->setName(name);

		if (target->isScript())
		{
			if (!parseScript(static_cast<ScriptDistTarget&>(*target), targetJson))
				continue;
		}
		else
		{
			if (!parseBundle(static_cast<BundleTarget&>(*target), targetJson))
				return false;
		}

		m_prototype.distribution.emplace_back(std::move(target));
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonProtoParser::parseScript(ScriptDistTarget& outScript, const Json& inNode)
{
	const std::string key{ "script" };

	if (StringList list; assignStringListFromConfig(list, inNode, key))
		outScript.addScripts(std::move(list));
	else if (std::string val; assignStringFromConfig(val, inNode, key))
		outScript.addScript(std::move(val));
	else
		return false;

	if (std::string val; assignStringFromConfig(val, inNode, "description"))
		outScript.setDescription(std::move(val));

	// if (!parsePlatformConfigExclusions(outProject, inNode))
	// 	return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonProtoParser::parseBundle(BundleTarget& outBundle, const Json& inNode)
{
	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "configuration"))
	{
		outBundle.setConfiguration(std::move(val));
		m_prototype.addRequiredBuildConfiguration(outBundle.configuration());
	}

	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "description"))
		outBundle.setDescription(std::move(val));

	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "outDir"))
		outBundle.setOutDir(std::move(val));

	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "mainProject"))
		outBundle.setMainProject(std::move(val));

	if (bool val; parseKeyFromConfig(val, inNode, "includeDependentSharedLibraries"))
		outBundle.setIncludeDependentSharedLibraries(val);

	if (StringList list; assignStringListFromConfig(list, inNode, "projects"))
	{
		outBundle.addProjects(std::move(list));
	}
	else
	{
		Diagnostic::error("{}: Distribution bundle '{}' was found without 'projects'", m_filename, outBundle.name());
		return false;
	}

	if (StringList list; assignStringListFromConfig(list, inNode, "include"))
		outBundle.addIncludes(std::move(list));

	if (StringList list; assignStringListFromConfig(list, inNode, "exclude"))
		outBundle.addExcludes(std::move(list));

#if defined(CHALET_LINUX)
	return parseBundleLinux(outBundle, inNode);
#elif defined(CHALET_MACOS)
	return parseBundleMacOS(outBundle, inNode);
#elif defined(CHALET_WIN32)
	return parseBundleWindows(outBundle, inNode);
#else
	#error "Unrecognized platform"
	return false;
#endif
}

/*****************************************************************************/
bool BuildJsonProtoParser::parseBundleLinux(BundleTarget& outBundle, const Json& inNode)
{
	if (!inNode.contains("linux"))
		return true;

	const Json& linuxNode = inNode.at("linux");
	if (!linuxNode.is_object())
	{
		Diagnostic::error("{}: '{}.linux' must be an object.", m_filename, kKeyDistribution);
		return false;
	}

	BundleLinux linuxBundle;

	int assigned = 0;
	if (std::string val; m_buildJson.assignStringAndValidate(val, linuxNode, "icon"))
	{
		linuxBundle.setIcon(std::move(val));
		assigned++;
	}

	if (std::string val; m_buildJson.assignStringAndValidate(val, linuxNode, "desktopEntry"))
	{
		linuxBundle.setDesktopEntry(std::move(val));
		assigned++;
	}

	if (assigned == 0)
		return false; // not an error

	if (assigned == 1)
	{
		Diagnostic::error("{}: '{bundle}.linux.icon' & '{bundle}.linux.desktopEntry' are both required.",
			m_filename,
			fmt::arg("bundle", kKeyDistribution));
		return false;
	}

	outBundle.setLinuxBundle(std::move(linuxBundle));

	return true;
}

/*****************************************************************************/
bool BuildJsonProtoParser::parseBundleMacOS(BundleTarget& outBundle, const Json& inNode)
{
	if (!inNode.contains("macos"))
		return true;

	const Json& macosNode = inNode.at("macos");
	if (!macosNode.is_object())
	{
		Diagnostic::error("{}: '{}.macos' must be an object.", m_filename, kKeyDistribution);
		return false;
	}

	BundleMacOS macosBundle;

	if (std::string val; m_buildJson.assignStringAndValidate(val, macosNode, "bundleName"))
		macosBundle.setBundleName(std::move(val));

	if (std::string val; m_buildJson.assignStringAndValidate(val, macosNode, "icon"))
		macosBundle.setIcon(std::move(val));

	const std::string kInfoPropertyList{ "infoPropertyList" };
	if (macosNode.contains(kInfoPropertyList))
	{
		auto& infoPlistNode = macosNode.at(kInfoPropertyList);
		if (infoPlistNode.is_object())
		{
			macosBundle.setInfoPropertyListContent(infoPlistNode.dump());
		}
		else
		{
			if (std::string val; m_buildJson.assignStringAndValidate(val, macosNode, kInfoPropertyList))
			{
				macosBundle.setInfoPropertyList(std::move(val));
			}
		}
	}

	const std::string kDmg{ "dmg" };
	if (macosNode.contains(kDmg))
	{
		const Json& dmg = macosNode.at(kDmg);

		macosBundle.setMakeDmg(true);
		const std::string kBackground{ "background" };

		if (dmg.contains(kBackground))
		{
			const Json& dmgBackground = dmg.at(kBackground);
			if (dmgBackground.is_object())
			{
				if (std::string val; m_buildJson.assignStringAndValidate(val, dmgBackground, "1x"))
					macosBundle.setDmgBackground1x(std::move(val));

				if (std::string val; m_buildJson.assignStringAndValidate(val, dmgBackground, "2x"))
					macosBundle.setDmgBackground2x(std::move(val));
			}
			else
			{
				if (std::string val; m_buildJson.assignStringAndValidate(val, dmg, kBackground))
					macosBundle.setDmgBackground1x(std::move(val));
			}
		}
	}

	outBundle.setMacosBundle(std::move(macosBundle));

	return true;
}

/*****************************************************************************/
bool BuildJsonProtoParser::parseBundleWindows(BundleTarget& outBundle, const Json& inNode)
{
	if (!inNode.contains("windows"))
		return true;

	const Json& windowsNode = inNode.at("windows");
	if (!windowsNode.is_object())
	{
		Diagnostic::error("{}: '{}.windows' must be an object.", m_filename, kKeyDistribution);
		return false;
	}

	BundleWindows windowsBundle;

	if (std::string val; m_buildJson.assignStringAndValidate(val, windowsNode, "nsisScript"))
		windowsBundle.setNsisScript(std::move(val));

	// int assigned = 0;
	// if (std::string val; m_buildJson.assignStringAndValidate(val, windowsNode, "icon"))
	// {
	// 	windowsBundle.setIcon(val);
	// 	assigned++;
	// }

	// if (assigned == 0)
	// 	return false;

	outBundle.setWindowsBundle(std::move(windowsBundle));

	return true;
}

/*****************************************************************************/
bool BuildJsonProtoParser::parseExternalDependencies(const Json& inNode)
{
	// don't care if there aren't any dependencies
	if (!inNode.contains(kKeyExternalDependencies))
		return true;

	const Json& externalDependencies = inNode.at(kKeyExternalDependencies);
	if (!externalDependencies.is_object() || externalDependencies.size() == 0)
	{
		Diagnostic::error("{}: '{}' must contain at least one external dependency.", m_filename, kKeyExternalDependencies);
		return false;
	}

	BuildDependencyType type = BuildDependencyType::Git;
	for (auto& [name, dependencyJson] : externalDependencies.items())
	{
		auto dependency = IBuildDependency::make(type, m_inputs, m_prototype);
		dependency->setName(name);

		if (!parseGitDependency(static_cast<GitDependency&>(*dependency), dependencyJson))
			return false;

		m_prototype.externalDependencies.emplace_back(std::move(dependency));
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonProtoParser::parseGitDependency(GitDependency& outDependency, const Json& inNode)
{
	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "repository"))
		outDependency.setRepository(std::move(val));
	else
	{
		Diagnostic::error("{}: 'repository' is required for all  external dependencies.", m_filename);
		return false;
	}

	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "branch"))
		outDependency.setBranch(std::move(val));

	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "tag"))
		outDependency.setTag(std::move(val));
	else if (m_buildJson.assignStringAndValidate(val, inNode, "commit"))
	{
		if (!outDependency.tag().empty())
		{
			Diagnostic::error("{}: Dependencies cannot contain both 'tag' and 'commit'. Found in '{}'", m_filename, outDependency.repository());
			return false;
		}

		outDependency.setCommit(std::move(val));
	}

	if (bool val = false; m_buildJson.assignFromKey(val, inNode, "submodules"))
		outDependency.setSubmodules(val);

	return true;
}

/*****************************************************************************/
/*****************************************************************************/
bool BuildJsonProtoParser::assignStringFromConfig(std::string& outVariable, const Json& inNode, const std::string& inKey, const std::string& inDefault)
{
	bool res = m_buildJson.assignStringAndValidate(outVariable, inNode, inKey, inDefault);

	const auto& platform = m_inputs.platform();

	res |= m_buildJson.assignStringAndValidate(outVariable, inNode, fmt::format("{}.{}", inKey, platform), inDefault);

	for (auto& notPlatform : m_inputs.notPlatforms())
	{
		res |= m_buildJson.assignStringAndValidate(outVariable, inNode, fmt::format("{}.!{}", inKey, notPlatform), inDefault);
	}

	return res;
}

/*****************************************************************************/
bool BuildJsonProtoParser::assignStringListFromConfig(StringList& outList, const Json& inNode, const std::string& inKey)
{
	bool res = m_buildJson.assignStringListAndValidate(outList, inNode, inKey);

	const auto& platform = m_inputs.platform();

	res |= m_buildJson.assignStringListAndValidate(outList, inNode, fmt::format("{}.{}", inKey, platform));

	for (auto& notPlatform : m_inputs.notPlatforms())
	{
		res |= m_buildJson.assignStringListAndValidate(outList, inNode, fmt::format("{}.!{}", inKey, notPlatform));
	}

	return res;
}

}
