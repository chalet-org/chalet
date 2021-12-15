/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

// Note: mind the order
#include "State/BuildState.hpp"
#include "Json/JsonFile.hpp"
//
#include "BuildJson/BuildJsonParser.hpp"
#include "BuildJson/SchemaBuildJson.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/Platform.hpp"
#include "State/BuildInfo.hpp"
#include "State/Dependency/BuildDependencyType.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/StatePrototype.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
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
	m_chaletJson(inPrototype.chaletJson()),
	m_filename(inPrototype.filename()),
	m_state(inState),
	kKeyTargets("targets"),
	kKeyAbstracts("abstracts"),
	m_debugIdentifier("debug"),
	m_notPlatforms(Platform::notPlatforms()),
	m_platform(Platform::platform())
{
}

/*****************************************************************************/
BuildJsonParser::~BuildJsonParser() = default;

/*****************************************************************************/
bool BuildJsonParser::serialize()
{
	// Timer timer;
	// Diagnostic::infoEllipsis("Reading Build File [{}]", m_filename);

	const Json& jRoot = m_chaletJson.json;
	if (!serializeFromJsonRoot(jRoot))
	{
		Diagnostic::error("{}: There was an error parsing the file.", m_filename);
		return false;
	}

	if (!validBuildRequested())
	{
		const auto& buildConfiguration = m_state.info.buildConfigurationNoAssert();
		Diagnostic::error("{}: No valid targets to build for '{}' configuration. Check usage of 'condition' property", m_filename, buildConfiguration);
		return false;
	}

	if (!validRunTargetRequestedFromInput())
	{
		Diagnostic::error("{}: Run target of '{}' is either: not a valid project name, is excluded from the build configuration '{}' or excluded on this platform.", m_filename, m_inputs.runTarget(), m_state.configuration.name());
		return false;
	}

	// TODO: Check custom configurations - if both lto & debug info / profiling are enabled, throw error (lto wipes out debug/profiling symbols)

	// Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::serializeFromJsonRoot(const Json& inJson)
{
	if (!parseTarget(inJson))
		return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::validBuildRequested() const
{
	int count = 0;
	for (auto& target : m_state.targets)
	{
		count++;

		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (project.language() == CodeLanguage::None)
			{
				Diagnostic::error("{}: All targets must have 'language' defined, but '{}' was found without one.", m_filename, project.name());
				return false;
			}
		}
	}
	return count > 0;
}

/*****************************************************************************/
bool BuildJsonParser::validRunTargetRequestedFromInput() const
{
	const auto& inputRunTarget = m_inputs.runTarget();
	if (inputRunTarget.empty())
		return true;

	for (auto& target : m_state.targets)
	{
		auto& name = target->name();
		if (name != inputRunTarget)
			continue;

		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (project.isExecutable())
				return true;
		}
		else if (target->isCMake())
		{
			auto& project = static_cast<const CMakeTarget&>(*target);
			if (!project.runExecutable().empty())
				return true;
		}
		else if (target->isScript())
		{
			return true;
		}
	}

	return false;
}

/*****************************************************************************/
bool BuildJsonParser::parseTarget(const Json& inNode)
{
	if (!inNode.contains(kKeyTargets))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_filename, kKeyTargets);
		return false;
	}

	const Json& targets = inNode.at(kKeyTargets);
	if (!targets.is_object() || targets.size() == 0)
	{
		Diagnostic::error("{}: '{}' must contain at least one target.", m_filename, kKeyTargets);
		return false;
	}

	if (inNode.contains(kKeyAbstracts))
	{
		const Json& abstracts = inNode.at(kKeyAbstracts);
		for (auto& [name, templateJson] : abstracts.items())
		{
			if (m_abstractSourceTarget.find(name) == m_abstractSourceTarget.end())
			{
				auto abstract = std::make_unique<SourceTarget>(m_state);
				if (!parseSourceTarget(*abstract, templateJson, true))
				{
					Diagnostic::error("{}: Error parsing the '{}' abstract project.", m_filename, name);
					return false;
				}

				m_abstractSourceTarget.emplace(name, std::move(abstract));
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

		if (m_abstractSourceTarget.find(name) == m_abstractSourceTarget.end())
		{
			auto abstract = std::make_unique<SourceTarget>(m_state);
			if (!parseSourceTarget(*abstract, abstractJson, true))
			{
				Diagnostic::error("{}: Error parsing the '{}' abstract project.", m_filename, name);
				return false;
			}

			m_abstractSourceTarget.emplace(name, std::move(abstract));
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

		std::string extends{ "*" };
		if (targetJson.is_object())
		{
			m_chaletJson.assignFromKey(extends, targetJson, "extends");
		}

		BuildTargetType type = BuildTargetType::Project;
		if (std::string val; m_chaletJson.assignFromKey(val, targetJson, "kind"))
		{
			if (String::equals("cmakeProject", val))
			{
				type = BuildTargetType::CMake;
			}
			else if (String::equals("chaletProject", val))
			{
				type = BuildTargetType::SubChalet;
			}
			else if (String::equals("script", val))
			{
				type = BuildTargetType::Script;
			}
		}

		BuildTarget target;
		if (type == BuildTargetType::Project && m_abstractSourceTarget.find(extends) != m_abstractSourceTarget.end())
		{
			target = std::make_unique<SourceTarget>(*m_abstractSourceTarget.at(extends)); // Note: copy ctor
		}
		else
		{
			if (type == BuildTargetType::Project && !String::equals('*', extends))
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
			if (!parseScriptTarget(static_cast<ScriptBuildTarget&>(*target), targetJson))
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
			if (!parseCMakeTarget(static_cast<CMakeTarget&>(*target), targetJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' target of type 'CMake'.", m_filename, name);
				return false;
			}
		}
		else
		{
			if (!parseSourceTarget(static_cast<SourceTarget&>(*target), targetJson))
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
bool BuildJsonParser::parseSourceTarget(SourceTarget& outTarget, const Json& inNode, const bool inAbstract) const
{
	if (!parseTargetCondition(outTarget, inNode))
		return true; // true to skip project

	if (std::string val; parseKeyFromConfig(val, inNode, "description"))
		outTarget.setDescription(std::move(val));

	if (std::string val; m_chaletJson.assignFromKey(val, inNode, "kind"))
		outTarget.setKind(val);

	// if (std::string val; m_chaletJson.assignFromKey(val, inNode, "name"))
	// 	outTarget.setName(val);

	if (std::string val; parseKeyFromConfig(val, inNode, "language"))
		outTarget.setLanguage(val);

	if (!parseFilesAndLocation(outTarget, inNode, inAbstract))
		return false;

	if (!parseRunTargetProperties(outTarget, inNode))
		return false;

	{
		const auto compilerSettings{ "settings" };
		if (inNode.contains(compilerSettings))
		{
			const Json& jCompilerSettings = inNode.at(compilerSettings);
			if (jCompilerSettings.contains("Cxx"))
			{
				const Json& node = jCompilerSettings.at("Cxx");
				if (!parseCompilerSettingsCxx(outTarget, node))
					return false;
			}
		}

		const auto compilerSettingsCpp = fmt::format("{}:Cxx", compilerSettings);
		if (inNode.contains(compilerSettingsCpp))
		{
			const Json& node = inNode.at(compilerSettingsCpp);
			if (!parseCompilerSettingsCxx(outTarget, node))
				return false;
		}
	}

	if (!inAbstract)
	{
		// Do some final error checking here

		if (outTarget.kind() == ProjectKind::None)
		{
			Diagnostic::error("{}: project '{}' must contain 'kind'.", m_filename, outTarget.name());
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseScriptTarget(ScriptBuildTarget& outTarget, const Json& inNode) const
{
	if (!parseTargetCondition(outTarget, inNode))
		return true;

	if (std::string val; parseKeyFromConfig(val, inNode, "file"))
		outTarget.setFile(std::move(val));
	else
		return false;

	if (std::string val; parseKeyFromConfig(val, inNode, "description"))
		outTarget.setDescription(std::move(val));

	if (!parseRunTargetProperties(outTarget, inNode))
		return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseSubChaletTarget(SubChaletTarget& outTarget, const Json& inNode) const
{
	if (!parseTargetCondition(outTarget, inNode))
		return true;

	if (std::string val; m_chaletJson.assignFromKey(val, inNode, "location"))
		outTarget.setLocation(std::move(val));
	else
		return false;

	if (std::string val; parseKeyFromConfig(val, inNode, "description"))
		outTarget.setDescription(std::move(val));

	if (std::string val; m_chaletJson.assignFromKey(val, inNode, "buildFile"))
		outTarget.setBuildFile(std::move(val));

	if (bool val = false; m_chaletJson.assignFromKey(val, inNode, "recheck"))
		outTarget.setRecheck(val);

	if (bool val = false; m_chaletJson.assignFromKey(val, inNode, "rebuild"))
		outTarget.setRebuild(val);

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseCMakeTarget(CMakeTarget& outTarget, const Json& inNode) const
{
	if (!parseTargetCondition(outTarget, inNode))
		return true;

	if (std::string val; m_chaletJson.assignFromKey(val, inNode, "location"))
		outTarget.setLocation(std::move(val));
	else
		return false;

	if (std::string val; parseKeyFromConfig(val, inNode, "description"))
		outTarget.setDescription(std::move(val));

	if (std::string val; m_chaletJson.assignFromKey(val, inNode, "buildFile"))
		outTarget.setBuildFile(std::move(val));

	if (bool val = false; m_chaletJson.assignFromKey(val, inNode, "recheck"))
		outTarget.setRecheck(val);

	if (bool val = false; m_chaletJson.assignFromKey(val, inNode, "rebuild"))
		outTarget.setRebuild(val);

	if (std::string val; parseKeyFromConfig(val, inNode, "toolset"))
		outTarget.setToolset(std::move(val));

	if (StringList list; m_chaletJson.assignStringListAndValidate(list, inNode, "defines"))
		outTarget.addDefines(std::move(list));

	if (std::string val; parseKeyFromConfig(val, inNode, "runExecutable"))
		outTarget.setRunExecutable(std::move(val));

	if (!parseRunTargetProperties(outTarget, inNode))
		return false;

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseTargetCondition(IBuildTarget& outTarget, const Json& inNode) const
{
	const auto& buildConfiguration = m_state.info.buildConfigurationNoAssert();
	if (!buildConfiguration.empty())
	{
		if (std::string val; m_chaletJson.assignFromKey(val, inNode, "condition"))
		{
			outTarget.setIncludeInBuild(conditionIsValid(val));
		}
	}

	return outTarget.includeInBuild();
}

/*****************************************************************************/
bool BuildJsonParser::parseRunTargetProperties(IBuildTarget& outTarget, const Json& inNode) const
{
	if (StringList list; parseStringListFromConfig(list, inNode, "runArguments"))
		outTarget.addRunArguments(std::move(list));

	if (bool val = false; parseKeyFromConfig(val, inNode, "runTarget"))
		outTarget.setRunTarget(val);

	if (StringList list; parseStringListFromConfig(list, inNode, "runDependencies"))
		outTarget.addRunDependencies(std::move(list));

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseCompilerSettingsCxx(SourceTarget& outTarget, const Json& inNode) const
{
	if (std::string val; parseKeyFromConfig(val, inNode, "windowsApplicationManifest"))
		outTarget.setWindowsApplicationManifest(std::move(val));
	else if (bool enabled = false; m_chaletJson.assignFromKey(enabled, inNode, "windowsApplicationManifest"))
		outTarget.setWindowsApplicationManifestGenerationEnabled(enabled);

	if (std::string val; parseKeyFromConfig(val, inNode, "windowsApplicationIcon"))
		outTarget.setWindowsApplicationIcon(std::move(val));

	if (std::string val; parseKeyFromConfig(val, inNode, "windowsSubSystem"))
		outTarget.setWindowsSubSystem(val);

	if (std::string val; parseKeyFromConfig(val, inNode, "windowsEntryPoint"))
		outTarget.setWindowsEntryPoint(val);

	// if (bool val = false; m_chaletJson.assignFromKey(val, inNode, "windowsOutputDef"))
	// 	outTarget.setWindowsOutputDef(val);

	if (std::string val; m_chaletJson.assignFromKey(val, inNode, "pch"))
		outTarget.setPch(std::move(val));

	if (bool val = false; parseKeyFromConfig(val, inNode, "rtti"))
		outTarget.setRtti(val);

	if (bool val = false; parseKeyFromConfig(val, inNode, "cppModules"))
		outTarget.setCppModules(val);

	if (bool val = false; parseKeyFromConfig(val, inNode, "cppCoroutines"))
		outTarget.setCppCoroutines(val);

	if (bool val = false; parseKeyFromConfig(val, inNode, "cppConcepts"))
		outTarget.setCppConcepts(val);

	if (bool val = false; parseKeyFromConfig(val, inNode, "exceptions"))
		outTarget.setExceptions(val);

	if (bool val = false; parseKeyFromConfig(val, inNode, "staticLinking"))
		outTarget.setStaticLinking(val);

	if (std::string val; parseKeyFromConfig(val, inNode, "threads"))
		outTarget.setThreadType(val);

	if (std::string val; parseKeyFromConfig(val, inNode, "cppStandard"))
		outTarget.setCppStandard(std::move(val));

	if (std::string val; parseKeyFromConfig(val, inNode, "cStandard"))
		outTarget.setCStandard(std::move(val));

	if (std::string val; parseKeyFromConfig(val, inNode, "warnings"))
		outTarget.setWarningPreset(std::move(val));
	else if (StringList list; parseStringListWithToolchain(list, inNode, "warnings"))
		outTarget.addWarnings(std::move(list));

	if (std::string val; parseKeyWithToolchain(val, inNode, "compileOptions"))
		outTarget.addCompileOptions(std::move(val));

	if (std::string val; parseKeyWithToolchain(val, inNode, "linkerOptions"))
		outTarget.addLinkerOptions(std::move(val));

	if (std::string val; parseKeyFromConfig(val, inNode, "linkerScript"))
		outTarget.setLinkerScript(std::move(val));

#if defined(CHALET_MACOS)
	if (StringList list; m_chaletJson.assignStringListAndValidate(list, inNode, "macosFrameworkPaths"))
		outTarget.addMacosFrameworkPaths(std::move(list));

	if (StringList list; m_chaletJson.assignStringListAndValidate(list, inNode, "macosFrameworks"))
		outTarget.addMacosFrameworks(std::move(list));
#endif

	if (StringList list; parseStringListWithToolchain(list, inNode, "defines"))
		outTarget.addDefines(std::move(list));

	if (StringList list; parseStringListWithToolchain(list, inNode, "links"))
		outTarget.addLinks(std::move(list));

	if (StringList list; parseStringListWithToolchain(list, inNode, "staticLinks"))
		outTarget.addStaticLinks(std::move(list));

	if (StringList list; parseStringListWithToolchain(list, inNode, "libDirs"))
		outTarget.addLibDirs(std::move(list));

	if (StringList list; parseStringListWithToolchain(list, inNode, "includeDirs"))
		outTarget.addIncludeDirs(std::move(list));

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseStringListWithToolchain(StringList& outList, const Json& inNode, const std::string& inKey) const
{
	chalet_assert(m_state.environment != nullptr, "");

	if (parseStringListFromConfig(outList, inNode, inKey))
	{
		return true;
	}
	else if (inNode.contains(inKey))
	{
		auto& innerNode = inNode.at(inKey);
		if (innerNode.is_object())
		{
			const auto& triple = m_state.info.targetArchitectureTriple();
			const auto& toolchainName = m_inputs.toolchainPreferenceName();

			bool res = parseStringListFromConfig(outList, innerNode, "*");

			if (triple != toolchainName)
				res |= parseStringListFromConfig(outList, innerNode, triple);

			res |= parseStringListFromConfig(outList, innerNode, toolchainName);

			return res;
		}
	}

	return false;
}

/*****************************************************************************/
bool BuildJsonParser::parseFilesAndLocation(SourceTarget& outTarget, const Json& inNode, const bool inAbstract) const
{
	bool locResult = parseProjectLocationOrFiles(outTarget, inNode);
	if (locResult && inAbstract)
	{
		Diagnostic::error("{}: '{}' cannot contain a location configuration.", m_filename, kKeyAbstracts);
		return false;
	}

	if (!locResult && !inAbstract)
	{
		Diagnostic::error("{}: 'location' or 'files' is required for project '{}', but was not found.", m_filename, outTarget.name());
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildJsonParser::parseProjectLocationOrFiles(SourceTarget& outTarget, const Json& inNode) const
{
	const std::string loc{ "location" };

	bool hasFiles = inNode.contains("files");

	if (!inNode.contains(loc))
	{
		if (hasFiles)
		{
			if (StringList list; parseStringListFromConfig(list, inNode, "files"))
				outTarget.addFiles(std::move(list));

			return true;
		}
		else
		{
			// No location or files
			return false;
		}
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
		if (StringList list; parseStringListFromConfig(list, node, "include"))
			outTarget.addLocations(std::move(list));
		else if (std::string val; parseKeyFromConfig(val, node, "include"))
			outTarget.addLocation(std::move(val));
		else
			return false;

		// exclude is optional
		if (StringList list; parseStringListFromConfig(list, node, "exclude"))
			outTarget.addLocationExcludes(std::move(list));
		else if (std::string val; parseKeyFromConfig(val, node, "exclude"))
			outTarget.addLocationExclude(std::move(val));
	}
	else if (StringList list; m_chaletJson.assignStringListAndValidate(list, inNode, loc))
		outTarget.addLocations(std::move(list));
	else if (std::string val; m_chaletJson.assignFromKey(val, inNode, loc))
		outTarget.addLocation(std::move(val));
	else
		return false;

	return true;
}

/*****************************************************************************/
/*****************************************************************************/
bool BuildJsonParser::parseStringListFromConfig(StringList& outList, const Json& inNode, const std::string& inKey) const
{
	bool res = m_chaletJson.assignStringListAndValidate(outList, inNode, inKey);

	const auto& platform = m_platform;

	res |= m_chaletJson.assignStringListAndValidate(outList, inNode, fmt::format("{}.{}", inKey, platform));

	const auto notSymbol = m_state.configuration.debugSymbols() ? "" : "!";
	res |= m_chaletJson.assignStringListAndValidate(outList, inNode, fmt::format("{}.{}{}", inKey, notSymbol, m_debugIdentifier));
	res |= m_chaletJson.assignStringListAndValidate(outList, inNode, fmt::format("{}.{}{}.{}", inKey, notSymbol, m_debugIdentifier, platform));
	res |= m_chaletJson.assignStringListAndValidate(outList, inNode, fmt::format("{}.{}.{}{}", inKey, platform, notSymbol, m_debugIdentifier));

	for (auto& notPlatform : m_notPlatforms)
	{
		res |= m_chaletJson.assignStringListAndValidate(outList, inNode, fmt::format("{}.!{}", inKey, notPlatform));
		res |= m_chaletJson.assignStringListAndValidate(outList, inNode, fmt::format("{}.!{}.{}{}", inKey, notPlatform, notSymbol, m_debugIdentifier));
		res |= m_chaletJson.assignStringListAndValidate(outList, inNode, fmt::format("{}.{}{}.!{}", inKey, notSymbol, m_debugIdentifier, notPlatform));
	}

	return res;
}

/*****************************************************************************/
bool BuildJsonParser::containsComplexKey(const Json& inNode, const std::string& inKey) const
{
	bool res = inNode.contains(inKey);

	const auto& platform = m_platform;

	res |= inNode.contains(fmt::format("{}.{}", inKey, platform));

	const auto notSymbol = m_state.configuration.debugSymbols() ? "" : "!";
	res |= inNode.contains(fmt::format("{}.{}{}", inKey, notSymbol, m_debugIdentifier));
	res |= inNode.contains(fmt::format("{}.{}{}.{}", inKey, notSymbol, m_debugIdentifier, platform));
	res |= inNode.contains(fmt::format("{}.{}.{}{}", inKey, platform, notSymbol, m_debugIdentifier));

	for (auto& notPlatform : m_notPlatforms)
	{
		res |= inNode.contains(fmt::format("{}.!{}", inKey, notPlatform));
		res |= inNode.contains(fmt::format("{}.!{}.{}{}", inKey, notPlatform, notSymbol, m_debugIdentifier));
		res |= inNode.contains(fmt::format("{}.{}{}.!{}", inKey, notSymbol, m_debugIdentifier, notPlatform));
	}

	return res;
}

/*****************************************************************************/
bool BuildJsonParser::conditionIsValid(const std::string& inContent) const
{
	const auto& platform = m_platform;

	if (String::equals(platform, inContent))
		return true;

	const auto notSymbol = m_state.configuration.debugSymbols() ? "" : "!";

	if (String::equals(fmt::format("{}{}", notSymbol, m_debugIdentifier), inContent))
		return true;

	if (String::equals(fmt::format("{}{}.{}", notSymbol, m_debugIdentifier, platform), inContent))
		return true;

	if (String::equals(fmt::format("{}.{}{}", platform, notSymbol, m_debugIdentifier), inContent))
		return true;

	for (auto& notPlatform : m_notPlatforms)
	{
		if (String::equals(fmt::format("!{}", notPlatform), inContent))
			return true;

		if (String::equals(fmt::format("!{}.{}{}", notSymbol, m_debugIdentifier, notPlatform), inContent))
			return true;
		if (String::equals(fmt::format("{}{}.!{}", notSymbol, m_debugIdentifier, notPlatform), inContent))
			return true;
	}

	return false;
}

}
