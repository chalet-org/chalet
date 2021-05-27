/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/BuildJsonParser.hpp"

#include "BuildJson/BuildJsonSchema.hpp"
#include "BuildJson/Bundle/BundleLinux.hpp"
#include "BuildJson/Bundle/BundleMacOS.hpp"
#include "BuildJson/Bundle/BundleWindows.hpp"
#include "Libraries/Format.hpp"
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
		return false;

	const auto& jRoot = m_buildJson->json;
	if (!serializeFromJsonRoot(jRoot))
	{
		Diagnostic::error(fmt::format("There was an error parsing {}", m_filename));
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

	std::string toHash = fmt::format("{jsonDump}_{buildConfiguration}",
		FMT_ARG(jsonDump),
		FMT_ARG(buildConfiguration));

	m_state.info.setHash(Hash::uint64(toHash));
	// LOG(hash);

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

	if (!parseEnvironment(inJson))
		return false;

	if (!makePathVariable())
		return false;

	if (!parseConfiguration(inJson))
		return false;

	if (!parseExternalDependencies(inJson))
		return false;

	if (!parseProjects(inJson))
		return false;

	if (!parseBundle(inJson))
		return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::validBuildRequested()
{
	int count = 0;
	for (auto& project : m_state.projects)
	{
		count++;

		if (project->hasScripts())
			continue;

		if (project->language() == CodeLanguage::None)
		{
			Diagnostic::errorAbort(fmt::format("{}: All projects must have 'language' defined, but '{}' was found without one.", m_filename, project->name()), "Error parsing file");
			return false;
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

	for (auto& project : m_state.projects)
	{
		if (!inputRunProject.empty() && project->name() == inputRunProject)
			return project->isExecutable();
	}

	return false;
}

/*****************************************************************************/
bool BuildJsonParser::parseRoot(const Json& inNode)
{
	if (!inNode.is_object())
	{
		Diagnostic::errorAbort(fmt::format("{}: Json root must be an object.", m_filename), "Error parsing file");
		return false;
	}

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "workspace"))
		m_state.info.setWorkspace(val);

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "version"))
		m_state.info.setVersion(val);

	return true;
}

/*****************************************************************************/
void BuildJsonParser::parseBuildConfiguration(const Json& inNode)
{
	if (inNode.contains(kKeyBundle))
	{
		// if the bundle is not an object, we'll error it in parseBundle
		const Json& bundle = inNode.at(kKeyBundle);
		if (bundle.is_object())
		{
			if (std::string val; m_buildJson->assignStringAndValidate(val, bundle, "configuration"))
				m_state.bundle.setConfiguration(val);
		}
	}

	const auto& buildConfiguration = m_inputs.buildConfiguration();
	if (buildConfiguration.empty())
	{
		const auto& bundleConfiguration = m_state.bundle.configuration();
		if (!bundleConfiguration.empty())
		{
			m_state.info.setBuildConfiguration(bundleConfiguration);
		}
		else
		{
			m_state.bundle.setConfiguration("Release");
			m_state.info.setBuildConfiguration("Release");
		}
	}
	else
	{
		m_state.info.setBuildConfiguration(buildConfiguration);
	}
}

/*****************************************************************************/
bool BuildJsonParser::parseEnvironment(const Json& inJson)
{
	// don't care
	if (!inJson.contains(kKeyEnvironment))
		return true;

	const Json& environment = inJson.at(kKeyEnvironment);
	if (!environment.is_object())
	{
		Diagnostic::errorAbort(fmt::format("{}: '{}' must be an object.", m_filename, kKeyEnvironment));
		return false;
	}

	if (std::string val; m_buildJson->assignStringAndValidate(val, environment, "externalDepDir"))
		m_state.environment.setExternalDepDir(val);

	if (StringList list; assignStringListFromConfig(list, environment, "path"))
		m_state.environment.addPaths(list);

	if (bool val = false; m_buildJson->assignFromKey(val, environment, "showCommands"))
		m_state.environment.setShowCommands(val);

	if (ushort val = 0; parseKeyFromConfig(val, environment, "maxJobs"))
		m_state.environment.setMaxJobs(val);

	return true;
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
		return setDefaultConfigurations(buildConfiguration);

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
		Diagnostic::errorAbort(fmt::format("{}: '{}' must contain at least one external dependency.", m_filename, kKeyExternalDependencies));
		return false;
	}

	for (auto& [name, dependencyJson] : externalDependencies.items())
	{
		auto dependency = std::make_unique<DependencyGit>(m_state.environment, m_inputs.buildFile());
		dependency->setName(name);

		if (!parseExternalDependency(*dependency, dependencyJson))
			return false;

		m_state.externalDependencies.push_back(std::move(dependency));
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseExternalDependency(DependencyGit& outDependency, const Json& inNode)
{
	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "repository"))
		outDependency.setRepository(val);
	else
	{
		Diagnostic::errorAbort(fmt::format("{}: 'repository' is required for all  external dependencies.", m_filename));
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
			Diagnostic::errorAbort(fmt::format("{}: Dependencies cannot contain both 'tag' and 'commit'. Found in '{}'", m_filename, outDependency.repository()));
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
	if (!inNode.contains(kKeyProjects))
	{
		Diagnostic::error(fmt::format("{}: '{}' is required, but was not found.", m_filename, kKeyProjects));
		return false;
	}

	const Json& projects = inNode.at(kKeyProjects);
	if (!projects.is_object() || projects.size() == 0)
	{
		Diagnostic::error(fmt::format("{}: '{}' must contain at least one project.", m_filename, kKeyProjects));
		return false;
	}

	// ProjectConfiguration allProjects(m_state.buildConfiguration(), m_state.environment);

	if (inNode.contains(kKeyTemplates))
	{
		const Json& templates = inNode.at(kKeyTemplates);
		for (auto& [name, templateJson] : templates.items())
		{
			if (m_abstractProjects.find(name) == m_abstractProjects.end())
			{
				auto abstractProject = std::make_unique<ProjectConfiguration>(m_state.info.buildConfiguration(), m_state.environment);
				if (!parseProject(*abstractProject, templateJson, true))
					return false;

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

	for (auto& [prefixedName, templateJson] : inNode.items())
	{
		std::string prefix{ fmt::format("{}:", kKeyTemplates) };
		if (!String::startsWith(prefix, prefixedName))
			continue;

		std::string name = prefixedName.substr(prefix.size());
		String::replaceAll(name, prefix, "");

		if (m_abstractProjects.find(name) == m_abstractProjects.end())
		{
			auto abstractProject = std::make_unique<ProjectConfiguration>(m_state.info.buildConfiguration(), m_state.environment);
			if (!parseProject(*abstractProject, templateJson, true))
				return false;

			m_abstractProjects.emplace(name, std::move(abstractProject));
		}
		else
		{
			// not sure if this would actually get triggered?
			Diagnostic::error(fmt::format("{}: project template '{}' already exists.", m_filename, name));
			return false;
		}
	}

	for (auto& [name, projectJson] : projects.items())
	{
		std::string extends{ "all" };
		if (projectJson.is_object())
		{
			m_buildJson->assignFromKey(extends, projectJson, "extends");
		}

		const bool isScript = containsKeyThatStartsWith(projectJson, "script");

		std::unique_ptr<ProjectConfiguration> project;
		if (!isScript && m_abstractProjects.find(extends) != m_abstractProjects.end())
		{
			project = std::make_unique<ProjectConfiguration>(*m_abstractProjects.at(extends)); // note: copy ctor
		}
		else
		{
			if (!isScript && !String::equals("all", extends))
			{
				Diagnostic::error(fmt::format("{}: project template '{}' is base of project '{}', but doesn't exist.", m_filename, extends, name));
				return false;
			}
			project = std::make_unique<ProjectConfiguration>(m_state.info.buildConfiguration(), m_state.environment);
		}
		project->setName(name);

		if (isScript)
		{
			if (!parseScript(*project, projectJson))
				continue;
		}
		else
		{
			if (!parseProject(*project, projectJson))
				return false;
		}

		if (!project->includeInBuild())
			continue;

		m_state.projects.push_back(std::move(project));
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseProject(ProjectConfiguration& outProject, const Json& inNode, const bool inAbstract)
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

	if (bool val = false; parseKeyFromConfig(val, inNode, "dumpAssembly"))
		outProject.setDumpAssembly(val);

	if (StringList list; assignStringListFromConfig(list, inNode, "runDependencies"))
		outProject.addRunDependencies(list);

	{
		const auto compilerSettings{ "compilerSettings" };
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

			{
				bool cmakeResult = parseProjectCmake(outProject, node);
				if (cmakeResult && inAbstract)
				{
					Diagnostic::errorAbort(fmt::format("{}: project template for '{}' cannot contain a cmake configuration.", m_filename, outProject.name()));
					return false;
				}

				// If it's a cmake project, ignore everything else and return
				if (cmakeResult)
				{
					auto& compilerConfig = m_state.compilerTools.getConfig(outProject.language());
					outProject.parseOutputFilename(compilerConfig);
					return true;
				}
			}

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

		auto& compilerConfig = m_state.compilerTools.getConfig(outProject.language());
		outProject.parseOutputFilename(compilerConfig);

		const bool isMsvc = compilerConfig.isMsvc();
		if (!isMsvc)
		{
			std::string libDir = compilerConfig.compilerPathLib();
			outProject.addLibDir(libDir);

			std::string includeDir = compilerConfig.compilerPathInclude();
			outProject.addIncludeDir(includeDir);
		}
	}

	// Resolve links from projects
	auto& projects = m_state.projects;
	for (auto& project : projects)
	{
		ProjectKind kind = project->kind();
		bool staticLib = kind == ProjectKind::StaticLibrary;

		outProject.resolveLinksFromProject(project->name(), staticLib);
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseScript(ProjectConfiguration& outProject, const Json& inNode)
{
	if (!parseProjectScripts(outProject, inNode))
		return false;

	if (std::string val; assignStringFromConfig(val, inNode, "description"))
		outProject.setDescription(val);

	// if (!parsePlatformConfigExclusions(outProject, inNode))
	// 	return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parsePlatformConfigExclusions(ProjectConfiguration& outProject, const Json& inNode)
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
bool BuildJsonParser::parseCompilerSettingsCxx(ProjectConfiguration& outProject, const Json& inNode)
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

	if (bool val = false; parseKeyFromConfig(val, inNode, "posixThreads"))
		outProject.setPosixThreads(val);

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
bool BuildJsonParser::parseFilesAndLocation(ProjectConfiguration& outProject, const Json& inNode, const bool inAbstract)
{
	bool locResult = parseProjectLocationOrFiles(outProject, inNode);
	if (locResult && inAbstract)
	{
		Diagnostic::error(fmt::format("{}: '{}' cannot contain a location configuration.", m_filename, kKeyTemplates));
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
bool BuildJsonParser::parseProjectScripts(ProjectConfiguration& outProject, const Json& inNode)
{
	const std::string key{ "script" };

	if (StringList list; assignStringListFromConfig(list, inNode, key))
		outProject.addScripts(list);
	else if (std::string val; assignStringFromConfig(val, inNode, key))
		outProject.addScript(val);
	else
		return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseProjectLocationOrFiles(ProjectConfiguration& outProject, const Json& inNode)
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
bool BuildJsonParser::parseProjectCmake(ProjectConfiguration& outProject, const Json& inNode)
{
	const std::string key{ "cmake" };

	if (!inNode.contains(key))
		return false;

	const Json& node = inNode.at(key);
	if (node.is_object())
	{
		// if it's an object, it must include "enabled"
		if (bool val = false; m_buildJson->assignFromKey(val, node, "enabled"))
			outProject.setCmake(val);

		if (bool val = false; m_buildJson->assignFromKey(val, node, "recheck"))
			outProject.setCmakeRecheck(val);

		//
		if (StringList list; m_buildJson->assignStringListAndValidate(list, node, "defines"))
			outProject.addCmakeDefines(list);
	}
	else if (bool val = false; m_buildJson->assignFromKey(val, inNode, key))
		outProject.setCmake(val);

	return outProject.cmake();
}

/*****************************************************************************/
bool BuildJsonParser::parseBundle(const Json& inNode)
{
	if (!inNode.contains(kKeyBundle))
	{
		m_state.bundle.setExists(false);
		return true;
	}

	const Json& bundle = inNode.at(kKeyBundle);
	if (!bundle.is_object())
	{
		Diagnostic::error(fmt::format("{}: '{}' must be an object.", m_filename, kKeyBundle));
		return false;
	}

	if (std::string val; m_buildJson->assignStringAndValidate(val, bundle, "appName"))
		m_state.bundle.setAppName(val);

	if (std::string val; m_buildJson->assignStringAndValidate(val, bundle, "shortDescription"))
		m_state.bundle.setShortDescription(val);

	if (std::string val; m_buildJson->assignStringAndValidate(val, bundle, "longDescription"))
		m_state.bundle.setLongDescription(val);

	if (std::string val; m_buildJson->assignStringAndValidate(val, bundle, "path"))
		m_state.bundle.setOutDir(val);

	if (StringList list; assignStringListFromConfig(list, bundle, "projects"))
		m_state.bundle.addProjects(list);

	if (StringList list; assignStringListFromConfig(list, bundle, "dependencies"))
		m_state.bundle.addDependencies(list);

	if (StringList list; assignStringListFromConfig(list, bundle, "exclude"))
		m_state.bundle.addExcludes(list);

#if defined(CHALET_LINUX)
	return parseBundleLinux(bundle);
#elif defined(CHALET_MACOS)
	return parseBundleMacOS(bundle);
#elif defined(CHALET_WIN32)
	return parseBundleWindows(bundle);
#else
	#error "Unrecognized platform"
	return false;
#endif
}

/*****************************************************************************/
bool BuildJsonParser::parseBundleLinux(const Json& inNode)
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

	m_state.bundle.setLinuxBundle(linuxBundle);

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseBundleMacOS(const Json& inNode)
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

	if (std::string val; m_buildJson->assignStringAndValidate(val, macosNode, "bundleIdentifier"))
	{
		macosBundle.setBundleIdentifier(val);
		assigned++;
	}

	if (std::string val; m_buildJson->assignStringAndValidate(val, macosNode, "icon"))
	{
		macosBundle.setIcon(val);
		assigned++;
	}

	if (std::string val; m_buildJson->assignStringAndValidate(val, macosNode, "infoPropertyList"))
	{
		macosBundle.setInfoPropertyList(val);
		assigned++;
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

	if (assigned >= 1 && assigned < 4)
	{
		Diagnostic::error(fmt::format("{}: '{bundle}.macos.bundleName', '{bundle}.macos.bundleIdentifier' & '{bundle}.macos.icon' are required.",
			m_filename,
			fmt::arg("bundle", kKeyBundle)));
		return false;
	}

	m_state.bundle.setMacosBundle(macosBundle);

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseBundleWindows(const Json& inNode)
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

	m_state.bundle.setWindowsBundle(windowsBundle);

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
