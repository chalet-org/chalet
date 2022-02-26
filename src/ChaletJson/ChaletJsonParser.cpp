/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

// Note: mind the order
#include "State/BuildState.hpp"
#include "Json/JsonFile.hpp"
//
#include "ChaletJson/ChaletJsonParser.hpp"
#include "ChaletJson/ChaletJsonSchema.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Compile/ToolchainTypes.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Core/Platform.hpp"
#include "State/BuildInfo.hpp"
#include "State/CentralState.hpp"
#include "State/Dependency/BuildDependencyType.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/ProcessBuildTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
namespace
{
/*****************************************************************************/
constexpr bool isUnread(JsonNodeReadStatus& inStatus)
{
	return inStatus == JsonNodeReadStatus::Unread;
}
}

/*****************************************************************************/
ChaletJsonParser::ChaletJsonParser(CentralState& inCentralState, BuildState& inState) :
	m_chaletJson(inCentralState.chaletJson()),
	m_state(inState),
	m_notPlatforms(Platform::notPlatforms()),
	m_platform(Platform::platform())
{
}

/*****************************************************************************/
ChaletJsonParser::~ChaletJsonParser() = default;

/*****************************************************************************/
bool ChaletJsonParser::serialize()
{
	// Timer timer;
	// Diagnostic::infoEllipsis("Reading Build File [{}]", m_chaletJson.filename());

	// m_toolchain = m_state.environment->identifier();
	// m_notToolchains = ToolchainTypes::getNotTypes(m_toolchain);

	const Json& jRoot = m_chaletJson.json;
	if (!serializeFromJsonRoot(jRoot))
	{
		Diagnostic::error("{}: There was an error parsing the file.", m_chaletJson.filename());
		return false;
	}

	if (!validBuildRequested())
	{
		const auto& buildConfiguration = m_state.info.buildConfigurationNoAssert();
		Diagnostic::error("{}: No valid targets to build for '{}' configuration. Check usage of 'condition' property", m_chaletJson.filename(), buildConfiguration);
		return false;
	}

	if (!validRunTargetRequestedFromInput())
	{
		Diagnostic::error("{}: Run target of '{}' is either: not a valid project name, is excluded from the build configuration '{}' or excluded on this platform.", m_chaletJson.filename(), m_state.inputs.runTarget(), m_state.configuration.name());
		return false;
	}

	// TODO: Check custom configurations - if both lto & debug info / profiling are enabled, throw error (lto wipes out debug/profiling symbols)

	// Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::serializeFromJsonRoot(const Json& inJson)
{
	if (!parseTarget(inJson))
		return false;

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::validBuildRequested() const
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
				Diagnostic::error("{}: All targets must have 'language' defined, but '{}' was found without one.", m_chaletJson.filename(), project.name());
				return false;
			}
		}
	}
	return count > 0;
}

/*****************************************************************************/
bool ChaletJsonParser::validRunTargetRequestedFromInput()
{
	const auto& inputRunTarget = m_state.inputs.runTarget();
	bool setRunTarget = inputRunTarget.empty();
	for (auto& target : m_state.targets)
	{
		auto& name = target->name();
		if (!setRunTarget && name != inputRunTarget)
			continue;

		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (project.isExecutable())
			{
				if (setRunTarget)
					m_state.inputs.setRunTarget(std::string(target->name()));

				return true;
			}
		}
		else if (target->isCMake())
		{
			auto& project = static_cast<const CMakeTarget&>(*target);
			if (!project.runExecutable().empty())
			{
				if (setRunTarget)
					m_state.inputs.setRunTarget(std::string(target->name()));

				return true;
			}
		}
		else if (target->isScript())
		{
			if (setRunTarget)
				m_state.inputs.setRunTarget(std::string(target->name()));

			return true;
		}
	}

	return false;
}

/*****************************************************************************/
bool ChaletJsonParser::parseTarget(const Json& inNode)
{
	if (!inNode.contains(Keys::Targets))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_chaletJson.filename(), Keys::Targets);
		return false;
	}

	const Json& targets = inNode.at(Keys::Targets);
	if (!targets.is_object() || targets.size() == 0)
	{
		Diagnostic::error("{}: '{}' must contain at least one target.", m_chaletJson.filename(), Keys::Targets);
		return false;
	}

	if (inNode.contains(Keys::Abstracts))
	{
		const Json& abstracts = inNode.at(Keys::Abstracts);
		for (auto& [name, templateJson] : abstracts.items())
		{
			if (m_abstractSourceTarget.find(name) == m_abstractSourceTarget.end())
			{
				auto abstract = std::make_unique<SourceTarget>(m_state);
				if (!parseSourceTarget(*abstract, templateJson))
				{
					Diagnostic::error("{}: Error parsing the '{}' abstract project.", m_chaletJson.filename(), name);
					return false;
				}

				m_abstractSourceTarget.emplace(name, std::move(abstract));
			}
			else
			{
				// not sure if this would actually get triggered?
				Diagnostic::error("{}: project template '{}' already exists.", m_chaletJson.filename(), name);
				return false;
			}
		}
	}

	for (auto& [prefixedName, abstractJson] : inNode.items())
	{
		std::string prefix{ fmt::format("{}:", Keys::Abstracts) };
		if (!String::startsWith(prefix, prefixedName))
			continue;

		if (!abstractJson.is_object())
		{
			Diagnostic::error("{}: abstract target '{}' must be an object.", m_chaletJson.filename(), prefixedName);
			return false;
		}

		std::string name = prefixedName.substr(prefix.size());
		String::replaceAll(name, prefix, "");

		if (m_abstractSourceTarget.find(name) == m_abstractSourceTarget.end())
		{
			auto abstract = std::make_unique<SourceTarget>(m_state);
			if (!parseSourceTarget(*abstract, abstractJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' abstract project.", m_chaletJson.filename(), name);
				return false;
			}

			m_abstractSourceTarget.emplace(name, std::move(abstract));
		}
		else
		{
			// not sure if this would actually get triggered?
			Diagnostic::error("{}: project template '{}' already exists.", m_chaletJson.filename(), name);
			return false;
		}
	}

	StringList sourceTargets{
		"executable",
		"staticLibrary",
		"sharedLibrary",
	};

	for (auto& [name, targetJson] : targets.items())
	{
		if (!targetJson.is_object())
		{
			Diagnostic::error("{}: target '{}' must be an object.", m_chaletJson.filename(), name);
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
			else if (String::equals("process", val))
			{
				type = BuildTargetType::Process;
			}
			else if (String::equals(sourceTargets, val))
			{}
			else
			{
				Diagnostic::error("{}: Found unrecognized target kind of '{}'", m_chaletJson.filename(), val);
				return false;
			}
		}
		else
		{
			Diagnostic::error("{}: Found unrecognized target of '{}'", m_chaletJson.filename(), name);
			return false;
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
				Diagnostic::error("{}: project template '{}' is base of project '{}', but doesn't exist.", m_chaletJson.filename(), extends, name);
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
				Diagnostic::error("{}: Error parsing the '{}' target of type 'chaletProject'.", m_chaletJson.filename(), name);
				return false;
			}
		}
		else if (target->isCMake())
		{
			if (!parseCMakeTarget(static_cast<CMakeTarget&>(*target), targetJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' target of type 'cmakeProject'.", m_chaletJson.filename(), name);
				return false;
			}
		}
		else if (target->isProcess())
		{
			if (!parseProcessTarget(static_cast<ProcessBuildTarget&>(*target), targetJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' target of type 'cmakeProject'.", m_chaletJson.filename(), name);
				return false;
			}
		}
		else
		{
			if (!parseSourceTarget(static_cast<SourceTarget&>(*target), targetJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' project target.", m_chaletJson.filename(), name);
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
bool ChaletJsonParser::parseSourceTarget(SourceTarget& outTarget, const Json& inNode) const
{
	if (!parseTargetCondition(outTarget, inNode))
		return true; // true to skip project

	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_object())
		{
			if (String::equals("files", key))
			{
				for (const auto& [k, v] : value.items())
				{
					JsonNodeReadStatus s = JsonNodeReadStatus::Unread;
					if (v.is_string())
					{
						std::string val;
						if (valueMatchesSearchKeyPattern(val, v, k, "include", s))
							outTarget.addFile(std::move(val));
						else if (isUnread(s) && valueMatchesSearchKeyPattern(val, v, k, "exclude", s))
							outTarget.addFileExclude(std::move(val));
					}
					else if (v.is_array())
					{
						StringList val;
						if (valueMatchesSearchKeyPattern(val, v, k, "include", s))
							outTarget.addFiles(std::move(val));
						else if (isUnread(s) && valueMatchesSearchKeyPattern(val, v, k, "exclude", s))
							outTarget.addFileExcludes(std::move(val));
					}
				}
			}
		}
		else if (value.is_string())
		{
			std::string val;
			if (valueMatchesSearchKeyPattern(val, value, key, "description", status))
				outTarget.setDescription(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "files", status))
				outTarget.addFile(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "language", status))
				outTarget.setLanguage(value.get<std::string>());
			else if (isUnread(status) && String::equals("kind", key))
				outTarget.setKind(value.get<std::string>());
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "files", status))
				outTarget.addFiles(std::move(val));
		}
	}

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

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::parseScriptTarget(ScriptBuildTarget& outTarget, const Json& inNode) const
{
	if (!parseTargetCondition(outTarget, inNode))
		return true;

	bool valid = false;
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			std::string val;
			if (valueMatchesSearchKeyPattern(val, value, key, "file", status))
			{
				outTarget.setFile(std::move(val));
				valid = true;
			}
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "description", status))
				outTarget.setDescription(std::move(val));
		}
	}

	if (!parseRunTargetProperties(outTarget, inNode))
		return false;

	return valid;
}

/*****************************************************************************/
bool ChaletJsonParser::parseSubChaletTarget(SubChaletTarget& outTarget, const Json& inNode) const
{
	if (!parseTargetCondition(outTarget, inNode))
		return true;

	bool valid = false;
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			std::string val;
			if (valueMatchesSearchKeyPattern(val, value, key, "location", status))
			{
				outTarget.setLocation(std::move(val));
				valid = true;
			}
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "description", status))
				outTarget.setDescription(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "buildFile", status))
				outTarget.setBuildFile(std::move(val));
		}
		else if (value.is_boolean())
		{
			bool val = false;
			if (valueMatchesSearchKeyPattern(val, value, key, "recheck", status))
				outTarget.setRecheck(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "rebuild", status))
				outTarget.setRebuild(val);
		}
	}

	return valid;
}

/*****************************************************************************/
bool ChaletJsonParser::parseCMakeTarget(CMakeTarget& outTarget, const Json& inNode) const
{
	if (!parseTargetCondition(outTarget, inNode))
		return true;

	bool valid = false;
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			std::string val;
			if (String::equals("location", key))
			{
				outTarget.setLocation(value.get<std::string>());
				valid = true;
			}
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "description", status))
				outTarget.setDescription(std::move(val));
			else if (isUnread(status) && String::equals("buildFile", key))
				outTarget.setBuildFile(value.get<std::string>());
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "toolset", status))
				outTarget.setToolset(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "runExecutable", status))
				outTarget.setRunExecutable(std::move(val));
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "defines", status))
				outTarget.addDefines(std::move(val));
		}
		else if (value.is_boolean())
		{
			bool val = false;
			if (valueMatchesSearchKeyPattern(val, value, key, "recheck", status))
				outTarget.setRecheck(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "rebuild", status))
				outTarget.setRebuild(val);
		}
	}

	if (!parseRunTargetProperties(outTarget, inNode))
		return false;

	return valid;
}

/*****************************************************************************/
bool ChaletJsonParser::parseProcessTarget(ProcessBuildTarget& outTarget, const Json& inNode) const
{
	if (!parseTargetCondition(outTarget, inNode))
		return true;

	bool valid = false;
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			std::string val;
			if (valueMatchesSearchKeyPattern(val, value, key, "path", status))
			{
				outTarget.setPath(std::move(val));
				valid = true;
			}
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "description", status))
				outTarget.setDescription(std::move(val));
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "arguments", status))
				outTarget.addDefaultRunArguments(std::move(val));
		}
	}

	return valid;
}

/*****************************************************************************/
bool ChaletJsonParser::parseTargetCondition(IBuildTarget& outTarget, const Json& inNode) const
{
	const auto& buildConfiguration = m_state.info.buildConfigurationNoAssert();
	if (!buildConfiguration.empty())
	{
		if (std::string val; m_chaletJson.assignFromKey(val, inNode, "condition"))
			outTarget.setIncludeInBuild(conditionIsValid(val));
	}

	return outTarget.includeInBuild();
}

/*****************************************************************************/
bool ChaletJsonParser::parseRunTargetProperties(IBuildTarget& outTarget, const Json& inNode) const
{
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "defaultRunArguments", status))
				outTarget.addDefaultRunArguments(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "copyFilesOnRun", status))
				outTarget.addCopyFilesOnRun(std::move(val));
		}
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::parseCompilerSettingsCxx(SourceTarget& outTarget, const Json& inNode) const
{
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			std::string val;
			if (valueMatchesSearchKeyPattern(val, value, key, "windowsApplicationManifest", status))
				outTarget.setWindowsApplicationManifest(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "windowsApplicationIcon", status))
				outTarget.setWindowsApplicationIcon(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "windowsSubSystem", status))
				outTarget.setWindowsSubSystem(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "windowsEntryPoint", status))
				outTarget.setWindowsEntryPoint(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "pch", status))
				outTarget.setPch(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "cppStandard", status))
				outTarget.setCppStandard(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "cStandard", status))
				outTarget.setCStandard(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "warnings", status))
				outTarget.setWarningPreset(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "linkerScript", status))
				outTarget.setLinkerScript(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "inputCharset", status))
				outTarget.setInputCharset(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "executionCharset", status))
				outTarget.setExecutionCharset(std::move(val));
			//
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "compileOptions", status))
				outTarget.addCompileOptions(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "linkerOptions", status))
				outTarget.addLinkerOptions(std::move(val));
		}
		else if (value.is_boolean())
		{
			bool val = false;
			if (valueMatchesSearchKeyPattern(val, value, key, "windowsApplicationManifest", status))
				outTarget.setWindowsApplicationManifestGenerationEnabled(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "threads", status))
				outTarget.setThreads(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "treatWarningsAsErrors", status))
				outTarget.setTreatWarningsAsErrors(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "rtti", status))
				outTarget.setRtti(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "cppFilesystem", status))
				outTarget.setCppFilesystem(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "cppModules", status))
				outTarget.setCppModules(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "cppCoroutines", status))
				outTarget.setCppCoroutines(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "cppConcepts", status))
				outTarget.setCppConcepts(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "exceptions", status))
				outTarget.setExceptions(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "staticLinking", status))
				outTarget.setStaticLinking(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "fastMath", status))
				outTarget.setFastMath(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "mingwUnixSharedLibraryNamingConvention", status))
				outTarget.setMinGWUnixSharedLibraryNamingConvention(val);
			// else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "windowsOutputDef", status))
			// 	outTarget.setWindowsOutputDef(val);

			//
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "warnings", status))
				outTarget.addWarnings(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "defines", status))
				outTarget.addDefines(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "links", status))
				outTarget.addLinks(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "staticLinks", status))
				outTarget.addStaticLinks(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "libDirs", status))
				outTarget.addLibDirs(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "includeDirs", status))
				outTarget.addIncludeDirs(std::move(val));
#if defined(CHALET_MACOS)
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "macosFrameworkPaths", status))
				outTarget.addMacosFrameworkPaths(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "macosFrameworks", status))
				outTarget.addMacosFrameworks(std::move(val));
#endif
		}
		else if (value.is_object())
		{
			{
				std::string val;
				if (valueMatchesToolchainSearchPattern(val, value, key, "compileOptions", status))
					outTarget.addCompileOptions(std::move(val));
				else if (isUnread(status) && valueMatchesToolchainSearchPattern(val, value, key, "linkerOptions", status))
					outTarget.addLinkerOptions(std::move(val));
				else if (isUnread(status) && valueMatchesToolchainSearchPattern(val, value, key, "inputCharset", status))
					outTarget.setInputCharset(std::move(val));
				else if (isUnread(status) && valueMatchesToolchainSearchPattern(val, value, key, "executionCharset", status))
					outTarget.setExecutionCharset(std::move(val));
			}
			if (isUnread(status))
			{
				StringList val;
				if (valueMatchesToolchainSearchPattern(val, value, key, "warnings", status))
					outTarget.addWarnings(std::move(val));
				else if (isUnread(status) && valueMatchesToolchainSearchPattern(val, value, key, "defines", status))
					outTarget.addDefines(std::move(val));
				else if (isUnread(status) && valueMatchesToolchainSearchPattern(val, value, key, "links", status))
					outTarget.addLinks(std::move(val));
				else if (isUnread(status) && valueMatchesToolchainSearchPattern(val, value, key, "staticLinks", status))
					outTarget.addStaticLinks(std::move(val));
				else if (isUnread(status) && valueMatchesToolchainSearchPattern(val, value, key, "libDirs", status))
					outTarget.addLibDirs(std::move(val));
				else if (isUnread(status) && valueMatchesToolchainSearchPattern(val, value, key, "includeDirs", status))
					outTarget.addIncludeDirs(std::move(val));
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::conditionIsValid(const std::string& inContent) const
{
	auto split = String::split(inContent, '.');
	if (List::contains(split, fmt::format(".!{}", m_platform)))
		return false;

	for (auto& notPlatform : m_notPlatforms)
	{
		if (List::contains(split, notPlatform))
			return false;
	}

	/*if (List::contains(split, fmt::format(".!{}", m_toolchain)))
		return false;

	for (auto& notToolchain : m_notToolchains)
	{
		if (List::contains(split, notToolchain))
			return false;
	}*/

	// future
	// if (List::contains(split, fmt::format("!{}", m_state.environment->identifier())))
	// 	return false;

	const auto incDebug = m_state.configuration.debugSymbols() ? "!" : "";
	if (List::contains(split, fmt::format("{}debug", incDebug)))
		return false;

	// const auto incCi = Environment::isContinuousIntegrationServer() ? "" : "!";
	// if (!List::contains(split, fmt::format("{}ci", incCi)))
	// 	return false;

	return true;
}

}
