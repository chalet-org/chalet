/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

// Note: mind the order
#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "Json/JsonFile.hpp"
//
#include "BuildJson/BuildJsonParser.hpp"

#include "BuildJson/SchemaBuildJson.hpp"
#include "State/Bundle/BundleLinux.hpp"
#include "State/Bundle/BundleMacOS.hpp"
#include "State/Bundle/BundleWindows.hpp"
#include "State/Dependency/BuildDependencyType.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/StatePrototype.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/ProjectTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
BuildJsonParser::BuildJsonParser(const CommandLineInputs& inInputs, StatePrototype& inPrototype, BuildState& inState) :
	m_inputs(inInputs),
	m_buildJson(inPrototype.jsonFile()),
	m_filename(inPrototype.filename()),
	m_state(inState),
	kKeyAbstracts(inPrototype.kKeyAbstracts),
	kKeyTargets(inPrototype.kKeyTargets)
{
}

/*****************************************************************************/
BuildJsonParser::~BuildJsonParser() = default;

/*****************************************************************************/
bool BuildJsonParser::serialize()
{
	// Timer timer;
	// Diagnostic::infoEllipsis("Reading Build File [{}]", m_filename);

	const Json& jRoot = m_buildJson.json;
	if (!serializeFromJsonRoot(jRoot))
	{
		Diagnostic::error("{}: There was an error parsing the file.", m_filename);
		return false;
	}

	const auto& buildConfiguration = m_state.info.buildConfiguration();
	if (!validBuildRequested())
	{
		Diagnostic::error("{}: No valid projects to build in '{}' configuration. Check usage of 'onlyInConfiguration'", m_filename, buildConfiguration);
		return false;
	}

	if (!validRunProjectRequestedFromInput())
	{
		Diagnostic::error("{}: Requested runProject of '{}' was not a valid project name.", m_filename, m_inputs.runProject());
		return false;
	}

	// TODO: Check custom configurations - if both lto & debug info / profiling are enabled, throw error (lto wipes out debug/profiling symbols)

	// Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::serializeFromJsonRoot(const Json& inJson)
{
	if (!parseProjects(inJson))
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
				Diagnostic::error("{}: All projects must have 'language' defined, but '{}' was found without one.", m_filename, project.name());
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
		auto& name = target->name();
		bool isProjectFromArgs = !inputRunProject.empty() && name == inputRunProject;
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
			if (isProjectFromArgs && project.isExecutable())
				return true;
		}
		else if (target->isScript())
		{
			if (isProjectFromArgs)
				return true;
		}
	}

	return false;
}

/*****************************************************************************/
bool BuildJsonParser::parseProjects(const Json& inNode)
{
	if (!inNode.contains(kKeyTargets))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_filename, kKeyTargets);
		return false;
	}

	const Json& targets = inNode.at(kKeyTargets);
	if (!targets.is_object() || targets.size() == 0)
	{
		Diagnostic::error("{}: '{}' must contain at least one project.", m_filename, kKeyTargets);
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
					Diagnostic::error("{}: Error parsing the '{}' abstract project.", m_filename, name);
					return false;
				}

				m_abstractProjects.emplace(name, std::move(abstractProject));
			}
			else
			{
				// not sure if this would actually get triggered?
				Diagnostic::error("{}: project template '{}' already exists.", m_filename, name);
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
			Diagnostic::error("{}: abstract target '{}' must be an object.", m_filename, prefixedName);
			return false;
		}

		std::string name = prefixedName.substr(prefix.size());
		String::replaceAll(name, prefix, "");

		if (m_abstractProjects.find(name) == m_abstractProjects.end())
		{
			auto abstractProject = std::make_unique<ProjectTarget>(m_state);
			if (!parseProject(*abstractProject, abstractJson, true))
			{
				Diagnostic::error("{}: Error parsing the '{}' abstract project.", m_filename, name);
				return false;
			}

			m_abstractProjects.emplace(name, std::move(abstractProject));
		}
		else
		{
			// not sure if this would actually get triggered?
			Diagnostic::error("{}: project template '{}' already exists.", m_filename, name);
			return false;
		}
	}

	for (auto& [name, targetJson] : targets.items())
	{
		if (!targetJson.is_object())
		{
			Diagnostic::error("{}: target '{}' must be an object.", m_filename, name);
			return false;
		}

		std::string extends{ "all" };
		if (targetJson.is_object())
		{
			m_buildJson.assignFromKey(extends, targetJson, "extends");
		}

		BuildTargetType type = BuildTargetType::Project;
		if (m_buildJson.containsKeyThatStartsWith(targetJson, "script"))
		{
			type = BuildTargetType::Script;
		}
		else if (targetJson.contains("type"))
		{
			std::string val;
			m_buildJson.assignFromKey(val, targetJson, "type");
			if (String::equals("CMake", val))
			{
				type = BuildTargetType::CMake;
			}
			else if (String::equals("Chalet", val))
			{
				type = BuildTargetType::SubChalet;
			}
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
				Diagnostic::error("{}: project template '{}' is base of project '{}', but doesn't exist.", m_filename, extends, name);
				return false;
			}

			target = IBuildTarget::make(type, m_state);
		}
		target->setName(name);

		if (target->isScript())
		{
			// A script could be only for a specific platform
			if (!parseScript(static_cast<ScriptBuildTarget&>(*target), targetJson))
				continue;
		}
		else if (target->isSubChalet())
		{
			if (!parseSubChaletTarget(static_cast<SubChaletTarget&>(*target), targetJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' target of type 'Chalet'.", m_filename, name);
				return false;
			}
		}
		else if (target->isCMake())
		{
			if (!parseCMakeProject(static_cast<CMakeTarget&>(*target), targetJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' target of type 'CMake'.", m_filename, name);
				return false;
			}
		}
		else
		{
			if (!parseProject(static_cast<ProjectTarget&>(*target), targetJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' project target.", m_filename, name);
				return false;
			}
		}

		if (!target->includeInBuild())
			continue;

		m_state.targets.emplace_back(std::move(target));
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseProject(ProjectTarget& outProject, const Json& inNode, const bool inAbstract)
{
	if (!parsePlatformConfigExclusions(outProject, inNode))
		return true; // true to skip project

	if (std::string val; assignStringFromConfig(val, inNode, "description"))
		outProject.setDescription(std::move(val));

	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "kind"))
		outProject.setKind(val);

	// if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "name"))
	// 	outProject.setName(val);

	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "language"))
		outProject.setLanguage(val);

	if (!parseFilesAndLocation(outProject, inNode, inAbstract))
		return false;

	if (StringList list; assignStringListFromConfig(list, inNode, "runArguments"))
		outProject.addRunArguments(std::move(list));

	if (bool val = false; parseKeyFromConfig(val, inNode, "runProject"))
		outProject.setRunProject(val);

	if (StringList list; assignStringListFromConfig(list, inNode, "runDependencies"))
		outProject.addRunDependencies(std::move(list));

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
			Diagnostic::error("{}: project '{}' must contain 'kind'.", m_filename, outProject.name());
			return false;
		}

		if (!outProject.pch().empty() && !Commands::pathExists(outProject.pch()))
		{
			Diagnostic::error("{}: Precompiled header '{}' was not found.", m_filename, outProject.pch());
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseScript(ScriptBuildTarget& outScript, const Json& inNode)
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
bool BuildJsonParser::parseSubChaletTarget(SubChaletTarget& outProject, const Json& inNode)
{
	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "location"))
		outProject.setLocation(std::move(val));
	else
		return false;

	if (std::string val; assignStringFromConfig(val, inNode, "description"))
		outProject.setDescription(std::move(val));

	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "buildFile"))
		outProject.setBuildFile(std::move(val));

	if (bool val = false; m_buildJson.assignFromKey(val, inNode, "recheck"))
		outProject.setRecheck(val);

	if (!parsePlatformConfigExclusions(outProject, inNode))
		return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseCMakeProject(CMakeTarget& outProject, const Json& inNode)
{
	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "location"))
		outProject.setLocation(std::move(val));
	else
		return false;

	if (std::string val; assignStringFromConfig(val, inNode, "description"))
		outProject.setDescription(std::move(val));

	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "buildFile"))
		outProject.setBuildFile(std::move(val));

	if (bool val = false; m_buildJson.assignFromKey(val, inNode, "recheck"))
		outProject.setRecheck(val);

	if (std::string val; assignStringFromConfig(val, inNode, "toolset"))
		outProject.setToolset(std::move(val));

	if (StringList list; m_buildJson.assignStringListAndValidate(list, inNode, "defines"))
		outProject.addDefines(std::move(list));

	if (!parsePlatformConfigExclusions(outProject, inNode))
		return false;

	// If it's a cmake project, ignore everything else and return
	// if (cmakeResult)

	// auto& compilerConfig = m_state.toolchain.getConfig(outProject.language());
	// outProject.parseOutputFilename(compilerConfig);
	// return true;

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parsePlatformConfigExclusions(IBuildTarget& outProject, const Json& inNode)
{
	const auto& buildConfiguration = m_state.info.buildConfiguration();
	const auto& platform = m_inputs.platform();

	if (StringList list; m_buildJson.assignStringListAndValidate(list, inNode, "onlyInConfiguration"))
		outProject.setIncludeInBuild(List::contains(list, buildConfiguration));
	else if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "onlyInConfiguration"))
		outProject.setIncludeInBuild(val == buildConfiguration);

	if (StringList list; m_buildJson.assignStringListAndValidate(list, inNode, "notInConfiguration"))
		outProject.setIncludeInBuild(!List::contains(list, buildConfiguration));
	else if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "notInConfiguration"))
		outProject.setIncludeInBuild(val != buildConfiguration);

	if (StringList list; m_buildJson.assignStringListAndValidate(list, inNode, "onlyInPlatform"))
		outProject.setIncludeInBuild(List::contains(list, platform));
	else if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "onlyInPlatform"))
		outProject.setIncludeInBuild(val == platform);

	if (StringList list; m_buildJson.assignStringListAndValidate(list, inNode, "notInPlatform"))
		outProject.setIncludeInBuild(!List::contains(list, platform));
	else if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "notInPlatform"))
		outProject.setIncludeInBuild(val != platform);

	return outProject.includeInBuild();
}

/*****************************************************************************/
bool BuildJsonParser::parseCompilerSettingsCxx(ProjectTarget& outProject, const Json& inNode)
{
	if (std::string val; assignStringFromConfig(val, inNode, "windowsApplicationManifest"))
		outProject.setWindowsApplicationManifest(std::move(val));

	if (std::string val; assignStringFromConfig(val, inNode, "windowsApplicationIcon"))
		outProject.setWindowsApplicationIcon(std::move(val));

	if (bool val = false; m_buildJson.assignFromKey(val, inNode, "windowsPrefixOutputFilename"))
		outProject.setWindowsPrefixOutputFilename(val);

	if (bool val = false; m_buildJson.assignFromKey(val, inNode, "windowsOutputDef"))
		outProject.setWindowsOutputDef(val);

	if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, "pch"))
		outProject.setPch(std::move(val));

	if (bool val = false; parseKeyFromConfig(val, inNode, "objectiveCxx"))
		outProject.setObjectiveCxx(val);

	if (bool val = false; parseKeyFromConfig(val, inNode, "rtti"))
		outProject.setRtti(val);

	if (bool val = false; parseKeyFromConfig(val, inNode, "exceptions"))
		outProject.setExceptions(val);

	if (bool val = false; parseKeyFromConfig(val, inNode, "staticLinking"))
		outProject.setStaticLinking(val);

	if (std::string val; assignStringFromConfig(val, inNode, "threads"))
		outProject.setThreadType(val);

	if (std::string val; assignStringFromConfig(val, inNode, "cppStandard"))
		outProject.setCppStandard(std::move(val));

	if (std::string val; assignStringFromConfig(val, inNode, "cStandard"))
		outProject.setCStandard(std::move(val));

	if (StringList list; assignStringListFromConfig(list, inNode, "warnings"))
		outProject.addWarnings(std::move(list));
	else if (std::string val; assignStringFromConfig(val, inNode, "warnings"))
		outProject.setWarningPreset(std::move(val));

	if (StringList list; assignStringListFromConfig(list, inNode, "compileOptions"))
		outProject.addCompileOptions(std::move(list));

	if (std::string val; assignStringFromConfig(val, inNode, "linkerScript"))
		outProject.setLinkerScript(std::move(val));

	if (StringList list; assignStringListFromConfig(list, inNode, "linkerOptions"))
		outProject.addLinkerOptions(std::move(list));

#if defined(CHALET_MACOS)
	if (StringList list; m_buildJson.assignStringListAndValidate(list, inNode, "macosFrameworkPaths"))
		outProject.addMacosFrameworkPaths(std::move(list));

	if (StringList list; m_buildJson.assignStringListAndValidate(list, inNode, "macosFrameworks"))
		outProject.addMacosFrameworks(std::move(list));
#endif

	if (StringList list; assignStringListFromConfig(list, inNode, "defines"))
		outProject.addDefines(std::move(list));

	if (StringList list; assignStringListFromConfig(list, inNode, "links"))
		outProject.addLinks(std::move(list));

	if (StringList list; assignStringListFromConfig(list, inNode, "staticLinks"))
		outProject.addStaticLinks(std::move(list));

	if (StringList list; assignStringListFromConfig(list, inNode, "libDirs"))
		outProject.addLibDirs(std::move(list));

	if (StringList list; assignStringListFromConfig(list, inNode, "includeDirs"))
		outProject.addIncludeDirs(std::move(list));

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseFilesAndLocation(ProjectTarget& outProject, const Json& inNode, const bool inAbstract)
{
	bool locResult = parseProjectLocationOrFiles(outProject, inNode);
	if (locResult && inAbstract)
	{
		Diagnostic::error("{}: '{}' cannot contain a location configuration.", m_filename, kKeyAbstracts);
		return false;
	}

	if (!locResult && !inAbstract)
	{
		Diagnostic::error("{}: 'location' or 'files' is required for project '{}', but was not found.", m_filename, outProject.name());
		return false;
	}

	// if (StringList list; assignStringListFromConfig(list, inNode, "fileExtensions"))
	// 	outProject.addFileExtensions(list);

	if (!inAbstract && (outProject.fileExtensions().size() == 0 && outProject.files().size() == 0))
	{
		Diagnostic::error("{}: No file extensions set for project: {}\n  Aborting...", m_filename, outProject.name());
		return false;
	}

	/*if (!inAbstract && (outProject.files().size() > 0 && outProject.fileExtensions().size() > 0))
	{
		Diagnostic::warn("{}: 'fileExtensions' ignored since 'files' are explicitely declared in project: {}", m_filename, outProject.name());
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
				outProject.addFiles(std::move(list));

			return true;
		}
		else
			return false;
	}

	if (hasFiles)
	{
		Diagnostic::error("{}: Define either 'files' or 'location', not both.", m_filename);
		return false;
	}

	const Json& node = inNode.at(loc);
	if (node.is_object())
	{
		// include is mandatory
		if (StringList list; assignStringListFromConfig(list, node, "include"))
			outProject.addLocations(std::move(list));
		else if (std::string val; assignStringFromConfig(val, node, "include"))
			outProject.addLocation(std::move(val));
		else
			return false;

		// exclude is optional
		if (StringList list; assignStringListFromConfig(list, node, "exclude"))
			outProject.addLocationExcludes(std::move(list));
		else if (std::string val; assignStringFromConfig(val, node, "exclude"))
			outProject.addLocationExclude(std::move(val));
	}
	else if (StringList list; m_buildJson.assignStringListAndValidate(list, inNode, loc))
		outProject.addLocations(std::move(list));
	else if (std::string val; m_buildJson.assignStringAndValidate(val, inNode, loc))
		outProject.addLocation(std::move(val));
	else
		return false;

	return true;
}

/*****************************************************************************/
/*****************************************************************************/
bool BuildJsonParser::assignStringFromConfig(std::string& outVariable, const Json& inNode, const std::string& inKey, const std::string& inDefault)
{
	bool res = m_buildJson.assignStringAndValidate(outVariable, inNode, inKey, inDefault);

	const auto& platform = m_inputs.platform();

	res |= m_buildJson.assignStringAndValidate(outVariable, inNode, fmt::format("{}.{}", inKey, platform), inDefault);

	if (m_state.configuration.debugSymbols())
	{
		res |= m_buildJson.assignStringAndValidate(outVariable, inNode, fmt::format("{}:{}", inKey, m_debugIdentifier), inDefault);
		res |= m_buildJson.assignStringAndValidate(outVariable, inNode, fmt::format("{}:{}.{}", inKey, m_debugIdentifier, platform), inDefault);
	}
	else
	{
		res |= m_buildJson.assignStringAndValidate(outVariable, inNode, fmt::format("{}:!{}", inKey, m_debugIdentifier), inDefault);
		res |= m_buildJson.assignStringAndValidate(outVariable, inNode, fmt::format("{}:!{}.{}", inKey, m_debugIdentifier, platform), inDefault);
	}

	for (auto& notPlatform : m_inputs.notPlatforms())
	{
		res |= m_buildJson.assignStringAndValidate(outVariable, inNode, fmt::format("{}.!{}", inKey, notPlatform), inDefault);

		if (m_state.configuration.debugSymbols())
			res |= m_buildJson.assignStringAndValidate(outVariable, inNode, fmt::format("{}:{}.!{}", inKey, m_debugIdentifier, notPlatform), inDefault);
		else
			res |= m_buildJson.assignStringAndValidate(outVariable, inNode, fmt::format("{}:!{}.!{}", inKey, m_debugIdentifier, notPlatform), inDefault);
	}

	return res;
}

/*****************************************************************************/
bool BuildJsonParser::assignStringListFromConfig(StringList& outList, const Json& inNode, const std::string& inKey)
{
	bool res = m_buildJson.assignStringListAndValidate(outList, inNode, inKey);

	const auto& platform = m_inputs.platform();

	res |= m_buildJson.assignStringListAndValidate(outList, inNode, fmt::format("{}.{}", inKey, platform));

	if (m_state.configuration.debugSymbols())
	{
		res |= m_buildJson.assignStringListAndValidate(outList, inNode, fmt::format("{}:{}", inKey, m_debugIdentifier));
		res |= m_buildJson.assignStringListAndValidate(outList, inNode, fmt::format("{}:{}.{}", inKey, m_debugIdentifier, platform));
	}
	else
	{
		res |= m_buildJson.assignStringListAndValidate(outList, inNode, fmt::format("{}:!{}", inKey, m_debugIdentifier));
		res |= m_buildJson.assignStringListAndValidate(outList, inNode, fmt::format("{}:!{}.{}", inKey, m_debugIdentifier, platform));
	}

	for (auto& notPlatform : m_inputs.notPlatforms())
	{
		res |= m_buildJson.assignStringListAndValidate(outList, inNode, fmt::format("{}.!{}", inKey, notPlatform));

		if (m_state.configuration.debugSymbols())
			res |= m_buildJson.assignStringListAndValidate(outList, inNode, fmt::format("{}:{}.!{}", inKey, m_debugIdentifier, notPlatform));
		else
			res |= m_buildJson.assignStringListAndValidate(outList, inNode, fmt::format("{}:!{}.!{}", inKey, m_debugIdentifier, notPlatform));
	}

	return res;
}

/*****************************************************************************/
bool BuildJsonParser::containsComplexKey(const Json& inNode, const std::string& inKey)
{
	bool res = inNode.contains(inKey);

	const auto& platform = m_inputs.platform();

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

	for (auto& notPlatform : m_inputs.notPlatforms())
	{
		res |= inNode.contains(fmt::format("{}.!{}", inKey, notPlatform));

		if (m_state.configuration.debugSymbols())
			res |= inNode.contains(fmt::format("{}:{}.!{}", inKey, m_debugIdentifier, notPlatform));
		else
			res |= inNode.contains(fmt::format("{}:!{}.!{}", inKey, m_debugIdentifier, notPlatform));
	}

	return res;
}

}
