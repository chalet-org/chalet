/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/BuildJsonParser.hpp"

#include "BuildJson/BuildJsonSchema.hpp"
#include "Libraries/Format.hpp"
#include "State/Bundle/BundleLinux.hpp"
#include "State/Bundle/BundleMacOS.hpp"
#include "State/Bundle/BundleWindows.hpp"
#include "State/Dependency/BuildDependencyType.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
BuildJsonParser::BuildJsonParser(const CommandLineInputs& inInputs, BuildState& inState, std::string inFilename) :
	m_inputs(inInputs),
	m_state(inState),
	m_filename(std::move(inFilename)),
	m_buildJson(std::make_unique<JsonFile>(m_filename))
{
}

/*****************************************************************************/
BuildJsonParser::~BuildJsonParser() = default;

/*****************************************************************************/
bool BuildJsonParser::serialize()
{
	// Note: existence of m_filename is checked by Router (before the cache is made)

	Timer timer;
	Diagnostic::info(fmt::format("Reading Build File [{}]", m_filename), false);

	Json buildJsonSchema = Schema::getBuildJson();

	if (m_inputs.saveSchemaToFile())
	{
		JsonFile::saveToFile(buildJsonSchema, "schema/chalet.schema.json");
	}

	// TODO: schema versioning
	if (!m_buildJson->validate(std::move(buildJsonSchema)))
	{
		Diagnostic::error(fmt::format("{}: There was an error validating the file against its schema.", m_filename));
		return false;
	}

	const auto& jRoot = m_buildJson->json;
	if (!serializeFromJsonRoot(jRoot))
	{
		Diagnostic::error(fmt::format("{}: There was an error parsing the file.", m_filename));
		return false;
	}

	const auto& buildConfiguration = m_state.info.buildConfiguration();
	if (!validBuildRequested())
	{
		Diagnostic::error(fmt::format("{}: No valid projects to build in '{}' configuration. Check usage of 'onlyInConfiguration'", m_filename, buildConfiguration));
		return false;
	}

	if (!validRunProjectRequestedFromInput())
	{
		Diagnostic::error(fmt::format("{}: Requested runProject of '{}' was not a valid project name.", m_filename, m_inputs.runProject()));
		return false;
	}

	// TODO: Check custom configurations - if both lto & debug info / profiling are enabled, throw error (lto wipes out debug/profiling symbols)

	std::string jsonDump = jRoot.dump();
	const auto& hostArch = m_state.info.hostArchitectureString();
	const auto& targetArch = m_state.info.targetArchitectureString();

	std::string toHash = fmt::format("{jsonDump}_{hostArch}_{targetArch}_{buildConfiguration}",
		FMT_ARG(jsonDump),
		FMT_ARG(hostArch),
		FMT_ARG(targetArch),
		FMT_ARG(buildConfiguration));

	m_state.info.setHash(Hash::uint64(toHash));

	Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::serializeFromJsonRoot(const Json& inJson)
{
	// order is important!

	parseBuildConfiguration(inJson);

	if (!parseRoot(inJson))
		return false;

	if (!makePathVariable())
		return false;

	if (!parseConfiguration(inJson))
		return false;

	if (!parseExternalDependencies(inJson))
		return false;

	if (!parseProjects(inJson))
		return false;

	if (!parseDistribution(inJson))
		return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::validBuildRequested()
{
	int count = 0;
	for (auto& target : m_state.targets)
	{
		count++;

		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
			if (project.language() == CodeLanguage::None)
			{
				Diagnostic::error(fmt::format("{}: All projects must have 'language' defined, but '{}' was found without one.", m_filename, project.name()), "Error parsing file");
				return false;
			}
		}
	}
	return count > 0;
}

/*****************************************************************************/
bool BuildJsonParser::validRunProjectRequestedFromInput()
{
	const auto& inputRunProject = m_inputs.runProject();
	if (inputRunProject.empty())
		return true;

	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
			if (!inputRunProject.empty() && project.name() == inputRunProject)
				return project.isExecutable();
		}
	}

	return false;
}

/*****************************************************************************/
bool BuildJsonParser::parseRoot(const Json& inNode)
{
	if (!inNode.is_object())
	{
		Diagnostic::error(fmt::format("{}: Json root must be an object.", m_filename), "Error parsing file");
		return false;
	}

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "workspace"))
		m_state.info.setWorkspace(val);

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "version"))
		m_state.info.setVersion(val);

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "externalDepDir"))
		m_state.paths.setExternalDepDir(val);

	if (StringList list; assignStringListFromConfig(list, inNode, "path"))
		m_state.environment.addPaths(list);

	return true;
}

/*****************************************************************************/
void BuildJsonParser::parseBuildConfiguration(const Json& inNode)
{
	UNUSED(inNode);
	// if (inNode.contains(kKeyBundle))
	{
		// if the bundle is not an object, we'll error it in parseBundle
		// const Json& bundle = inNode.at(kKeyBundle);
		// if (bundle.is_object())
		// {
		// 	if (std::string val; m_buildJson->assignStringAndValidate(val, bundle, "configuration"))
		// 		m_state.bundle.setConfiguration(val);
		// }
	}

	const auto& buildConfiguration = m_inputs.buildConfiguration();
	if (buildConfiguration.empty())
	{
		// const auto& bundleConfiguration = m_state.bundle.configuration();
		// if (!bundleConfiguration.empty())
		// {
		// 	m_state.info.setBuildConfiguration(bundleConfiguration);
		// }
		// else
		{
			// m_state.bundle.setConfiguration("Release");
			m_state.info.setBuildConfiguration("Release");
		}
	}
	else
	{
		m_state.info.setBuildConfiguration(buildConfiguration);
	}
}

/*****************************************************************************/
bool BuildJsonParser::makePathVariable()
{
	auto rootPath = m_state.compilerTools.getRootPathVariable();
	auto pathVariable = m_state.environment.makePathVariable(rootPath);

	// LOG(pathVariable);

	Environment::set("PATH", pathVariable);
	// #if defined(CHALET_WIN32)
	// 	Environment::set("Path", pathVariable);
	// #endif

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseConfiguration(const Json& inNode)
{
	const auto& buildConfiguration = m_state.info.buildConfiguration();

	if (!inNode.contains(kKeyConfigurations))
	{
		bool result = setDefaultConfigurations(buildConfiguration);
		if (!result)
		{
			Diagnostic::error(fmt::format("{}: Error setting the build configuration to '{}'.", m_filename, buildConfiguration));
		}
		return result;
	}

	std::string typeError = fmt::format("{}: '{}' must be either an array of presets, or an object where key is name", m_filename, kKeyConfigurations);

	bool configFound = false;

	const Json& configurations = inNode.at(kKeyConfigurations);
	if (configurations.is_object())
	{
		for (auto& [name, config] : configurations.items())
		{
			if (!config.is_object())
			{
				Diagnostic::error(fmt::format("{}: configuration '{}' must be an object.", m_filename, name));
				return false;
			}

			if (name.empty())
			{
				Diagnostic::error(fmt::format("{}: '{}' cannot contain blank keys.", m_filename, kKeyConfigurations));
				return false;
			}

			if (name != buildConfiguration)
				continue;

			m_state.configuration.setName(name);

			if (std::string val; m_buildJson->assignStringAndValidate(val, config, "optimizations"))
				m_state.configuration.setOptimizations(val);

			if (bool val = false; m_buildJson->assignFromKey(val, config, "linkTimeOptimization"))
				m_state.configuration.setLinkTimeOptimization(val);

			if (bool val = false; m_buildJson->assignFromKey(val, config, "stripSymbols"))
				m_state.configuration.setStripSymbols(val);

			if (bool val = false; m_buildJson->assignFromKey(val, config, "debugSymbols"))
				m_state.configuration.setDebugSymbols(val);

			if (bool val = false; m_buildJson->assignFromKey(val, config, "enableProfiling"))
				m_state.configuration.setEnableProfiling(val);

			configFound = true;
			break;
		}
	}
	else if (configurations.is_array())
	{
		for (auto& config : configurations)
		{
			if (config.is_string())
			{
				if (config == buildConfiguration)
					return setDefaultConfigurations(config);
			}
			else
			{
				Diagnostic::error(typeError);
				return false;
			}
		}
	}
	else
	{
		Diagnostic::error(typeError);
		return false;
	}

	if (!configFound)
	{
		Diagnostic::error(fmt::format("{}: The configuration '{}' was not found.", m_filename, buildConfiguration));
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseExternalDependencies(const Json& inNode)
{
	// don't care if there are no dependencies
	if (!inNode.contains(kKeyExternalDependencies))
		return true;

	const Json& externalDependencies = inNode.at(kKeyExternalDependencies);
	if (!externalDependencies.is_object() || externalDependencies.size() == 0)
	{
		Diagnostic::error(fmt::format("{}: '{}' must contain at least one external dependency.", m_filename, kKeyExternalDependencies));
		return false;
	}

	BuildDependencyType type = BuildDependencyType::Git;
	for (auto& [name, dependencyJson] : externalDependencies.items())
	{
		auto dependency = IBuildDependency::make(type, m_state);
		dependency->setName(name);

		if (!parseGitDependency(static_cast<GitDependency&>(*dependency), dependencyJson))
			return false;

		m_state.externalDependencies.push_back(std::move(dependency));
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseGitDependency(GitDependency& outDependency, const Json& inNode)
{
	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "repository"))
		outDependency.setRepository(val);
	else
	{
		Diagnostic::error(fmt::format("{}: 'repository' is required for all  external dependencies.", m_filename));
		return false;
	}

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "branch"))
		outDependency.setBranch(val);

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "tag"))
		outDependency.setTag(val);
	else if (m_buildJson->assignStringAndValidate(val, inNode, "commit"))
	{
		if (!outDependency.tag().empty())
		{
			Diagnostic::error(fmt::format("{}: Dependencies cannot contain both 'tag' and 'commit'. Found in '{}'", m_filename, outDependency.repository()));
			return false;
		}

		outDependency.setCommit(val);
	}

	if (bool val = false; m_buildJson->assignFromKey(val, inNode, "submodules"))
		outDependency.setSubmodules(val);

	if (!outDependency.parseDestination())
		return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseProjects(const Json& inNode)
{
	if (!inNode.contains(kKeyTargets))
	{
		Diagnostic::error(fmt::format("{}: '{}' is required, but was not found.", m_filename, kKeyTargets));
		return false;
	}

	const Json& targets = inNode.at(kKeyTargets);
	if (!targets.is_object() || targets.size() == 0)
	{
		Diagnostic::error(fmt::format("{}: '{}' must contain at least one project.", m_filename, kKeyTargets));
		return false;
	}

	if (inNode.contains(kKeyAbstracts))
	{
		const Json& abstracts = inNode.at(kKeyAbstracts);
		for (auto& [name, templateJson] : abstracts.items())
		{
			if (m_abstractProjects.find(name) == m_abstractProjects.end())
			{
				auto abstractProject = std::make_unique<ProjectTarget>(m_state);
				if (!parseProject(*abstractProject, templateJson, true))
				{
					Diagnostic::error(fmt::format("{}: Error parsing the '{}' abstract project.", m_filename, name));
					return false;
				}

				m_abstractProjects.emplace(name, std::move(abstractProject));
			}
			else
			{
				// not sure if this would actually get triggered?
				Diagnostic::error(fmt::format("{}: project template '{}' already exists.", m_filename, name));
				return false;
			}
		}
	}

	for (auto& [prefixedName, abstractJson] : inNode.items())
	{
		std::string prefix{ fmt::format("{}:", kKeyAbstracts) };
		if (!String::startsWith(prefix, prefixedName))
			continue;

		if (!abstractJson.is_object())
		{
			Diagnostic::error(fmt::format("{}: abstract target '{}' must be an object.", m_filename, prefixedName));
			return false;
		}

		std::string name = prefixedName.substr(prefix.size());
		String::replaceAll(name, prefix, "");

		if (m_abstractProjects.find(name) == m_abstractProjects.end())
		{
			auto abstractProject = std::make_unique<ProjectTarget>(m_state);
			if (!parseProject(*abstractProject, abstractJson, true))
			{
				Diagnostic::error(fmt::format("{}: Error parsing the '{}' abstract project.", m_filename, name));
				return false;
			}

			m_abstractProjects.emplace(name, std::move(abstractProject));
		}
		else
		{
			// not sure if this would actually get triggered?
			Diagnostic::error(fmt::format("{}: project template '{}' already exists.", m_filename, name));
			return false;
		}
	}

	for (auto& [name, targetJson] : targets.items())
	{
		if (!targetJson.is_object())
		{
			Diagnostic::error(fmt::format("{}: target '{}' must be an object.", m_filename, name));
			return false;
		}

		std::string extends{ "all" };
		if (targetJson.is_object())
		{
			m_buildJson->assignFromKey(extends, targetJson, "extends");
		}

		BuildTargetType type = BuildTargetType::Project;
		if (containsKeyThatStartsWith(targetJson, "script"))
		{
			type = BuildTargetType::Script;
		}
		else if (targetJson.contains("cmake"))
		{
			type = BuildTargetType::CMake;
		}

		BuildTarget target;
		if (type == BuildTargetType::Project && m_abstractProjects.find(extends) != m_abstractProjects.end())
		{
			target = std::make_unique<ProjectTarget>(*m_abstractProjects.at(extends)); // note: copy ctor
		}
		else
		{
			if (type == BuildTargetType::Project && !String::equals("all", extends))
			{
				Diagnostic::error(fmt::format("{}: project template '{}' is base of project '{}', but doesn't exist.", m_filename, extends, name));
				return false;
			}

			target = IBuildTarget::makeBuild(type, m_state);
		}
		target->setName(name);

		if (target->isScript())
		{
			if (!parseScript(static_cast<ScriptTarget&>(*target), targetJson))
			{
				Diagnostic::error(fmt::format("{}: Error parsing the '{}' script.", m_filename, name));
				return false;
			}
		}
		else if (target->isCMake())
		{
			if (!parseCMakeProject(static_cast<CMakeTarget&>(*target), targetJson))
			{
				Diagnostic::error(fmt::format("{}: Error parsing the '{}' cmake project.", m_filename, name));
				return false;
			}
		}
		else
		{
			if (!parseProject(static_cast<ProjectTarget&>(*target), targetJson))
			{
				Diagnostic::error(fmt::format("{}: Error parsing the '{}' project.", m_filename, name));
				return false;
			}
		}

		if (!target->includeInBuild())
			continue;

		m_state.targets.push_back(std::move(target));
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseProject(ProjectTarget& outProject, const Json& inNode, const bool inAbstract)
{
	if (!parsePlatformConfigExclusions(outProject, inNode))
		return true; // true to skip project

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "kind"))
		outProject.setKind(val);

	// if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "name"))
	// 	outProject.setName(val);

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "language"))
		outProject.setLanguage(val);

	if (!parseFilesAndLocation(outProject, inNode, inAbstract))
		return false;

	if (StringList list; assignStringListFromConfig(list, inNode, "runArguments"))
		outProject.addRunArguments(list);

	if (bool val = false; parseKeyFromConfig(val, inNode, "runProject"))
		outProject.setRunProject(val);

	if (StringList list; assignStringListFromConfig(list, inNode, "runDependencies"))
		outProject.addRunDependencies(list);

	{
		const auto compilerSettings{ "settings" };
		/*if (inNode.contains(compilerSettings))
		{
			const Json& jCompilerSettings = inNode.at(compilerSettings);
			if (jCompilerSettings.contains("Cxx"))
			{
				const Json& node = jCompilerSettings.at("Cxx");
				if (!parseCompilerSettingsCxx(outProject, node))
					return false;
			}
		}*/

		const auto compilerSettingsCpp = fmt::format("{}:Cxx", compilerSettings);
		if (inNode.contains(compilerSettingsCpp))
		{
			const Json& node = inNode.at(compilerSettingsCpp);

			if (!parseCompilerSettingsCxx(outProject, node))
				return false;
		}
	}

	if (!inAbstract)
	{
		// Do some final error checking here

		if (outProject.kind() == ProjectKind::None)
		{
			Diagnostic::error(fmt::format("{}: project '{}' must contain 'kind'.", m_filename, outProject.name()));
			return false;
		}

		if (!outProject.pch().empty() && !Commands::pathExists(outProject.pch()))
		{
			Diagnostic::error(fmt::format("{}: Precompiled header '{}' was not found.", m_filename, outProject.pch()));
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseScript(ScriptTarget& outScript, const Json& inNode)
{
	const std::string key{ "script" };

	if (StringList list; assignStringListFromConfig(list, inNode, key))
		outScript.addScripts(list);
	else if (std::string val; assignStringFromConfig(val, inNode, key))
		outScript.addScript(val);
	else
		return false;

	if (std::string val; assignStringFromConfig(val, inNode, "description"))
		outScript.setDescription(val);

	// if (!parsePlatformConfigExclusions(outProject, inNode))
	// 	return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseCMakeProject(CMakeTarget& outProject, const Json& inNode)
{
	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "location"))
		outProject.setLocation(std::move(val));
	else
		return false;

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "buildScript"))
		outProject.setBuildScript(std::move(val));

	if (bool val = false; m_buildJson->assignFromKey(val, inNode, "recheck"))
		outProject.setRecheck(val);

	if (StringList list; m_buildJson->assignStringListAndValidate(list, inNode, "defines"))
		outProject.addDefines(list);

	if (std::string val; assignStringFromConfig(val, inNode, "description"))
		outProject.setDescription(val);

	if (std::string val; assignStringFromConfig(val, inNode, "toolset"))
		outProject.setToolset(std::move(val));

	if (!parsePlatformConfigExclusions(outProject, inNode))
		return false;

	// If it's a cmake project, ignore everything else and return
	// if (cmakeResult)

	// auto& compilerConfig = m_state.compilerTools.getConfig(outProject.language());
	// outProject.parseOutputFilename(compilerConfig);
	// return true;

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parsePlatformConfigExclusions(IBuildTarget& outProject, const Json& inNode)
{
	const auto& buildConfiguration = m_state.info.buildConfiguration();
	const auto& platform = m_state.info.platform();

	if (StringList list; m_buildJson->assignStringListAndValidate(list, inNode, "onlyInConfiguration"))
		outProject.setIncludeInBuild(List::contains(list, buildConfiguration));
	else if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "onlyInConfiguration"))
		outProject.setIncludeInBuild(val == buildConfiguration);

	if (StringList list; m_buildJson->assignStringListAndValidate(list, inNode, "notInConfiguration"))
		outProject.setIncludeInBuild(!List::contains(list, buildConfiguration));
	else if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "notInConfiguration"))
		outProject.setIncludeInBuild(val != buildConfiguration);

	if (StringList list; m_buildJson->assignStringListAndValidate(list, inNode, "onlyInPlatform"))
		outProject.setIncludeInBuild(List::contains(list, platform));
	else if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "onlyInPlatform"))
		outProject.setIncludeInBuild(val == platform);

	if (StringList list; m_buildJson->assignStringListAndValidate(list, inNode, "notInPlatform"))
		outProject.setIncludeInBuild(!List::contains(list, platform));
	else if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "notInPlatform"))
		outProject.setIncludeInBuild(val != platform);

	return outProject.includeInBuild();
}

/*****************************************************************************/
bool BuildJsonParser::parseCompilerSettingsCxx(ProjectTarget& outProject, const Json& inNode)
{
	if (bool val = false; m_buildJson->assignFromKey(val, inNode, "windowsPrefixOutputFilename"))
		outProject.setWindowsPrefixOutputFilename(val);

	if (bool val = false; m_buildJson->assignFromKey(val, inNode, "windowsOutputDef"))
		outProject.setWindowsOutputDef(val);

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "pch"))
		outProject.setPch(val);

	if (bool val = false; parseKeyFromConfig(val, inNode, "objectiveCxx"))
		outProject.setObjectiveCxx(val);

	if (bool val = false; parseKeyFromConfig(val, inNode, "rtti"))
		outProject.setRtti(val);

	if (bool val = false; parseKeyFromConfig(val, inNode, "staticLinking"))
		outProject.setStaticLinking(val);

	if (std::string val; assignStringFromConfig(val, inNode, "threads"))
		outProject.setThreadType(val);

	if (std::string val; assignStringFromConfig(val, inNode, "cppStandard"))
		outProject.setCppStandard(val);

	if (std::string val; assignStringFromConfig(val, inNode, "cStandard"))
		outProject.setCStandard(val);

	if (StringList list; assignStringListFromConfig(list, inNode, "warnings"))
		outProject.addWarnings(list);
	else if (std::string val; assignStringFromConfig(val, inNode, "warnings"))
		outProject.setWarningPreset(val);

	if (StringList list; assignStringListFromConfig(list, inNode, "compileOptions"))
		outProject.addCompileOptions(list);

	if (std::string list; assignStringFromConfig(list, inNode, "linkerScript"))
		outProject.setLinkerScript(list);

	if (StringList list; assignStringListFromConfig(list, inNode, "linkerOptions"))
		outProject.addLinkerOptions(list);

#if defined(CHALET_MACOS)
	if (StringList list; m_buildJson->assignStringListAndValidate(list, inNode, "macosFrameworkPaths"))
		outProject.addMacosFrameworkPaths(list);

	if (StringList list; m_buildJson->assignStringListAndValidate(list, inNode, "macosFrameworks"))
		outProject.addMacosFrameworks(list);
#endif

	if (StringList list; assignStringListFromConfig(list, inNode, "defines"))
		outProject.addDefines(list);

	if (StringList list; assignStringListFromConfig(list, inNode, "links"))
		outProject.addLinks(list);

	if (StringList list; assignStringListFromConfig(list, inNode, "staticLinks"))
		outProject.addStaticLinks(list);

	if (StringList list; assignStringListFromConfig(list, inNode, "libDirs"))
		outProject.addLibDirs(list);

	if (StringList list; assignStringListFromConfig(list, inNode, "includeDirs"))
		outProject.addIncludeDirs(list);

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseFilesAndLocation(ProjectTarget& outProject, const Json& inNode, const bool inAbstract)
{
	bool locResult = parseProjectLocationOrFiles(outProject, inNode);
	if (locResult && inAbstract)
	{
		Diagnostic::error(fmt::format("{}: '{}' cannot contain a location configuration.", m_filename, kKeyAbstracts));
		return false;
	}

	if (!locResult && !inAbstract)
	{
		Diagnostic::error(fmt::format("{}: 'location' or 'files' is required for project '{}', but was not found.", m_filename, outProject.name()));
		return false;
	}

	// if (StringList list; assignStringListFromConfig(list, inNode, "fileExtensions"))
	// 	outProject.addFileExtensions(list);

	if (!inAbstract && (outProject.fileExtensions().size() == 0 && outProject.files().size() == 0))
	{
		Diagnostic::error(fmt::format("{}: No file extensions set for project: {}\n  Aborting...", m_filename, outProject.name()));
		return false;
	}

	/*if (!inAbstract && (outProject.files().size() > 0 && outProject.fileExtensions().size() > 0))
	{
		Diagnostic::warn(fmt::format("{}: 'fileExtensions' ignored since 'files' are explicitely declared in project: {}", m_filename, outProject.name()));
	}*/

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseProjectLocationOrFiles(ProjectTarget& outProject, const Json& inNode)
{
	const std::string loc{ "location" };

	bool hasFiles = inNode.contains("files");

	if (!inNode.contains(loc))
	{
		if (hasFiles)
		{
			if (StringList list; assignStringListFromConfig(list, inNode, "files"))
				outProject.addFiles(list);

			return true;
		}
		else
			return false;
	}

	if (hasFiles)
	{
		Diagnostic::error(fmt::format("{}: Define either 'files' or 'location', not both.", m_filename));
		return false;
	}

	const Json& node = inNode.at(loc);
	if (node.is_object())
	{
		// include is mandatory
		if (StringList list; assignStringListFromConfig(list, node, "include"))
			outProject.addLocations(list);
		else if (std::string val; assignStringFromConfig(val, node, "include"))
			outProject.addLocation(val);
		else
			return false;

		// exclude is optional
		if (StringList list; assignStringListFromConfig(list, node, "exclude"))
			outProject.addLocationExcludes(list);
		else if (std::string val; assignStringFromConfig(val, node, "exclude"))
			outProject.addLocationExclude(val);
	}
	else if (StringList list; m_buildJson->assignStringListAndValidate(list, inNode, loc))
		outProject.addLocations(list);
	else if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, loc))
		outProject.addLocation(val);
	else
		return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseDistribution(const Json& inNode)
{
	if (!inNode.contains(kKeyDistribution))
		return true;

	const Json& distribution = inNode.at(kKeyDistribution);
	if (!distribution.is_object() || distribution.size() == 0)
	{
		Diagnostic::error(fmt::format("{}: '{}' must contain at least one bundle or script.", m_filename, kKeyDistribution));
		return false;
	}

	for (auto& [name, targetJson] : distribution.items())
	{
		if (!targetJson.is_object())
		{
			Diagnostic::error(fmt::format("{}: distribution bundle '{}' must be an object.", m_filename, name));
			return false;
		}

		BuildTargetType type = BuildTargetType::DistributionBundle;
		if (containsKeyThatStartsWith(targetJson, "script"))
		{
			type = BuildTargetType::Script;
		}

		DistributionTarget target = IBuildTarget::makeBundle(type, m_state);
		target->setName(name);

		if (target->isScript())
		{
			if (!parseScript(static_cast<ScriptTarget&>(*target), targetJson))
				continue;
		}
		else
		{
			if (!parseBundle(static_cast<BundleTarget&>(*target), targetJson))
				return false;
		}

		// if (!target->includeInBuild())
		// 	continue;

		m_state.distribution.push_back(std::move(target));
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseBundle(BundleTarget& outBundle, const Json& inNode)
{
	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "description"))
		outBundle.setDescription(val);

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "outDir"))
		outBundle.setOutDir(val);

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "mainProject"))
		outBundle.setMainProject(val);

	if (bool val; parseKeyFromConfig(val, inNode, "includeDependentSharedLibraries"))
		outBundle.setIncludeDependentSharedLibraries(val);

	if (StringList list; assignStringListFromConfig(list, inNode, "projects"))
		outBundle.addProjects(list);

	if (StringList list; assignStringListFromConfig(list, inNode, "dependencies"))
		outBundle.addDependencies(list);

	if (StringList list; assignStringListFromConfig(list, inNode, "exclude"))
		outBundle.addExcludes(list);

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
bool BuildJsonParser::parseBundleLinux(BundleTarget& outBundle, const Json& inNode)
{
	if (!inNode.contains("linux"))
		return true;

	const Json& linuxNode = inNode.at("linux");
	if (!linuxNode.is_object())
	{
		Diagnostic::error(fmt::format("{}: '{}.linux' must be an object.", m_filename, kKeyBundle));
		return false;
	}

	BundleLinux linuxBundle;

	int assigned = 0;
	if (std::string val; m_buildJson->assignStringAndValidate(val, linuxNode, "icon"))
	{
		linuxBundle.setIcon(val);
		assigned++;
	}

	if (std::string val; m_buildJson->assignStringAndValidate(val, linuxNode, "desktopEntry"))
	{
		linuxBundle.setDesktopEntry(val);
		assigned++;
	}

	if (assigned == 0)
		return false; // not an error

	if (assigned == 1)
	{
		Diagnostic::error(fmt::format("{}: '{bundle}.linux.icon' & '{bundle}.linux.desktopEntry' are both required.",
			m_filename,
			fmt::arg("bundle", kKeyBundle)));
		return false;
	}

	outBundle.setLinuxBundle(std::move(linuxBundle));

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseBundleMacOS(BundleTarget& outBundle, const Json& inNode)
{
	if (!inNode.contains("macos"))
		return true;

	const Json& macosNode = inNode.at("macos");
	if (!macosNode.is_object())
	{
		Diagnostic::error(fmt::format("{}: '{}.macos' must be an object.", m_filename, kKeyBundle));
		return false;
	}

	BundleMacOS macosBundle;

	int assigned = 0;
	if (std::string val; m_buildJson->assignStringAndValidate(val, macosNode, "bundleName"))
	{
		macosBundle.setBundleName(val);
		assigned++;
	}

	if (std::string val; m_buildJson->assignStringAndValidate(val, macosNode, "icon"))
		macosBundle.setIcon(val);

	const std::string kInfoPropertyList{ "infoPropertyList" };
	if (macosNode.contains(kInfoPropertyList))
	{
		auto& infoPlistNode = macosNode.at(kInfoPropertyList);
		if (infoPlistNode.is_object())
		{
			macosBundle.setInfoPropertyListContent(infoPlistNode.dump());
			assigned++;
		}
		else
		{
			if (std::string val; m_buildJson->assignStringAndValidate(val, macosNode, kInfoPropertyList))
			{
				macosBundle.setInfoPropertyList(val);
				assigned++;
			}
		}
	}

	if (bool val = false; m_buildJson->assignFromKey(val, macosNode, "makeDmg"))
		macosBundle.setMakeDmg(val);

	if (StringList list; m_buildJson->assignStringListAndValidate(list, macosNode, "dylibs"))
		macosBundle.addDylibs(list);

	const std::string kDmgBackground{ "dmgBackground" };
	if (macosNode.contains(kDmgBackground))
	{
		const Json& dmgBackground = macosNode[kDmgBackground];
		if (dmgBackground.is_object())
		{
			if (std::string val; m_buildJson->assignStringAndValidate(val, dmgBackground, "1x"))
				macosBundle.setDmgBackground1x(val);

			if (std::string val; m_buildJson->assignStringAndValidate(val, dmgBackground, "2x"))
				macosBundle.setDmgBackground2x(val);
		}
		else
		{
			if (std::string val; m_buildJson->assignStringAndValidate(val, macosNode, kDmgBackground))
				macosBundle.setDmgBackground1x(val);
		}
	}

	if (assigned == 0)
		return false; // not an error

	if (assigned >= 1 && assigned < 2)
	{
		Diagnostic::error(fmt::format("{}: '{bundle}.macos.bundleName' is required.",
			m_filename,
			fmt::arg("bundle", kKeyBundle)));
		return false;
	}

	outBundle.setMacosBundle(std::move(macosBundle));

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseBundleWindows(BundleTarget& outBundle, const Json& inNode)
{
	if (!inNode.contains("windows"))
		return true;

	const Json& windowsNode = inNode.at("windows");
	if (!windowsNode.is_object())
	{
		Diagnostic::error(fmt::format("{}: '{}.windows' must be an object.", m_filename, kKeyBundle));
		return false;
	}

	BundleWindows windowsBundle;

	int assigned = 0;
	if (std::string val; m_buildJson->assignStringAndValidate(val, windowsNode, "icon"))
	{
		windowsBundle.setIcon(val);
		assigned++;
	}

	if (std::string val; m_buildJson->assignStringAndValidate(val, windowsNode, "manifest"))
	{
		windowsBundle.setManifest(val);
		assigned++;
	}

	if (assigned == 0)
		return false;

	if (assigned == 1)
	{
		Diagnostic::error(fmt::format("{}: '{bundle}.windows.icon' & '{bundle}.windows.manifest' are both required.",
			m_filename,
			fmt::arg("bundle", kKeyBundle)));
		return false;
	}

	outBundle.setWindowsBundle(std::move(windowsBundle));

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::setDefaultConfigurations(const std::string& inConfig)
{
	if (String::equals("Release", inConfig))
	{
		m_state.configuration.setName(inConfig);
		m_state.configuration.setOptimizations("3");
		m_state.configuration.setLinkTimeOptimization(true);
		m_state.configuration.setStripSymbols(true);
	}
	else if (String::equals("Debug", inConfig))
	{
		m_state.configuration.setName(inConfig);
		m_state.configuration.setOptimizations("0");
		m_state.configuration.setDebugSymbols(true);
	}
	// these two are the same as cmake
	else if (String::equals("RelWithDebInfo", inConfig))
	{
		m_state.configuration.setName(inConfig);
		m_state.configuration.setOptimizations("2");
		m_state.configuration.setDebugSymbols(true);
		m_state.configuration.setLinkTimeOptimization(true);
	}
	else if (String::equals("MinSizeRel", inConfig))
	{
		m_state.configuration.setName(inConfig);
		m_state.configuration.setOptimizations("size");
		// m_state.configuration.setLinkTimeOptimization(true);
		m_state.configuration.setStripSymbols(true);
	}
	else if (String::equals("Profile", inConfig))
	{
		m_state.configuration.setName(inConfig);
		m_state.configuration.setOptimizations("0");
		m_state.configuration.setDebugSymbols(true);
		m_state.configuration.setEnableProfiling(true);
	}
	else
	{
		Diagnostic::error(fmt::format("{}: An invalid build configuration ({}) was requested. Expected: Release, Debug, RelWithDebInfo, MinSizeRel, Profile", m_filename, inConfig));
		return false;
	}

	return true;
}

/*****************************************************************************/
/*****************************************************************************/
bool BuildJsonParser::assignStringFromConfig(std::string& outVariable, const Json& inNode, const std::string& inKey, const std::string& inDefault)
{
	bool res = m_buildJson->assignStringAndValidate(outVariable, inNode, inKey, inDefault);

	const auto& platform = m_state.info.platform();

	res |= m_buildJson->assignStringAndValidate(outVariable, inNode, fmt::format("{}.{}", inKey, platform), inDefault);

	if (m_state.configuration.debugSymbols())
	{
		res |= m_buildJson->assignStringAndValidate(outVariable, inNode, fmt::format("{}:{}", inKey, m_debugIdentifier), inDefault);
		res |= m_buildJson->assignStringAndValidate(outVariable, inNode, fmt::format("{}:{}.{}", inKey, m_debugIdentifier, platform), inDefault);
	}
	else
	{
		res |= m_buildJson->assignStringAndValidate(outVariable, inNode, fmt::format("{}:!{}", inKey, m_debugIdentifier), inDefault);
		res |= m_buildJson->assignStringAndValidate(outVariable, inNode, fmt::format("{}:!{}.{}", inKey, m_debugIdentifier, platform), inDefault);
	}

	for (auto& notPlatform : m_state.info.notPlatforms())
	{
		res |= m_buildJson->assignStringAndValidate(outVariable, inNode, fmt::format("{}.!{}", inKey, notPlatform), inDefault);

		if (m_state.configuration.debugSymbols())
			res |= m_buildJson->assignStringAndValidate(outVariable, inNode, fmt::format("{}:{}.!{}", inKey, m_debugIdentifier, notPlatform), inDefault);
		else
			res |= m_buildJson->assignStringAndValidate(outVariable, inNode, fmt::format("{}:!{}.!{}", inKey, m_debugIdentifier, notPlatform), inDefault);
	}

	return res;
}

/*****************************************************************************/
bool BuildJsonParser::assignStringListFromConfig(StringList& outList, const Json& inNode, const std::string& inKey)
{
	bool res = m_buildJson->assignStringListAndValidate(outList, inNode, inKey);

	const auto& platform = m_state.info.platform();

	res |= m_buildJson->assignStringListAndValidate(outList, inNode, fmt::format("{}.{}", inKey, platform));

	if (m_state.configuration.debugSymbols())
	{
		res |= m_buildJson->assignStringListAndValidate(outList, inNode, fmt::format("{}:{}", inKey, m_debugIdentifier));
		res |= m_buildJson->assignStringListAndValidate(outList, inNode, fmt::format("{}:{}.{}", inKey, m_debugIdentifier, platform));
	}
	else
	{
		res |= m_buildJson->assignStringListAndValidate(outList, inNode, fmt::format("{}:!{}", inKey, m_debugIdentifier));
		res |= m_buildJson->assignStringListAndValidate(outList, inNode, fmt::format("{}:!{}.{}", inKey, m_debugIdentifier, platform));
	}

	for (auto& notPlatform : m_state.info.notPlatforms())
	{
		res |= m_buildJson->assignStringListAndValidate(outList, inNode, fmt::format("{}.!{}", inKey, notPlatform));

		if (m_state.configuration.debugSymbols())
			res |= m_buildJson->assignStringListAndValidate(outList, inNode, fmt::format("{}:{}.!{}", inKey, m_debugIdentifier, notPlatform));
		else
			res |= m_buildJson->assignStringListAndValidate(outList, inNode, fmt::format("{}:!{}.!{}", inKey, m_debugIdentifier, notPlatform));
	}

	return res;
}

/*****************************************************************************/
bool BuildJsonParser::containsComplexKey(const Json& inNode, const std::string& inKey)
{
	bool res = inNode.contains(inKey);

	const auto& platform = m_state.info.platform();

	res |= inNode.contains(fmt::format("{}.{}", inKey, platform));

	if (m_state.configuration.debugSymbols())
	{
		res |= inNode.contains(fmt::format("{}:{}", inKey, m_debugIdentifier));
		res |= inNode.contains(fmt::format("{}:{}.{}", inKey, m_debugIdentifier, platform));
	}
	else
	{
		res |= inNode.contains(fmt::format("{}:!{}", inKey, m_debugIdentifier));
		res |= inNode.contains(fmt::format("{}:!{}.{}", inKey, m_debugIdentifier, platform));
	}

	for (auto& notPlatform : m_state.info.notPlatforms())
	{
		res |= inNode.contains(fmt::format("{}.!{}", inKey, notPlatform));

		if (m_state.configuration.debugSymbols())
			res |= inNode.contains(fmt::format("{}:{}.!{}", inKey, m_debugIdentifier, notPlatform));
		else
			res |= inNode.contains(fmt::format("{}:!{}.!{}", inKey, m_debugIdentifier, notPlatform));
	}

	return res;
}

/*****************************************************************************/
bool BuildJsonParser::containsKeyThatStartsWith(const Json& inNode, const std::string& inFind)
{
	bool res = false;

	for (auto& [key, value] : inNode.items())
	{
		res |= String::startsWith(inFind, key);
	}

	return res;
}

}
