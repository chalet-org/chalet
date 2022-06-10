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
#include "State/TargetMetadata.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonKeys.hpp"

#include "State/Target/CMakeTarget.hpp"
#include "State/Target/ProcessBuildTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"

#include "State/Distribution/BundleArchiveTarget.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/MacosDiskImageTarget.hpp"
#include "State/Distribution/ProcessDistTarget.hpp"
#include "State/Distribution/ScriptDistTarget.hpp"

namespace chalet
{
namespace
{
/*****************************************************************************/
constexpr bool isUnread(JsonNodeReadStatus& inStatus)
{
	return inStatus == JsonNodeReadStatus::Unread;
}
/*****************************************************************************/
constexpr bool isInvalid(JsonNodeReadStatus& inStatus)
{
	return inStatus == JsonNodeReadStatus::Invalid;
}
}

/*****************************************************************************/
ChaletJsonParser::ChaletJsonParser(CentralState& inCentralState, BuildState& inState) :
	m_chaletJson(inCentralState.chaletJson()),
	m_centralState(inCentralState),
	m_state(inState),
	kValidPlatforms(Platform::validPlatforms()),
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

	if (m_state.inputs.routeWillRun())
	{
		if (!validRunTargetRequestedFromInput())
		{
			Diagnostic::error("{}: Run target of '{}' is either: not a valid project name, is excluded from the build configuration '{}' or excluded on this platform.", m_chaletJson.filename(), m_state.inputs.runTarget(), m_state.configuration.name());
			return false;
		}

		// do after run target is validated
		m_centralState.getRunTargetArguments();
	}

	// Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::serializeFromJsonRoot(const Json& inJson)
{
	if (!parseRoot(inJson))
		return false;

	if (m_centralState.inputs().route() != Route::Configure)
	{
		if (!parseDistribution(inJson))
			return false;
	}

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
	auto runTarget = m_state.inputs.runTarget();
	// for (int i = 0; i < 2; ++i)
	{
		bool setRunTarget = runTarget.empty();
		for (auto& target : m_state.targets)
		{
			auto& name = target->name();
			if (!setRunTarget && name != runTarget)
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

		// runTarget = std::string();
	}

	return false;
}

/*****************************************************************************/
bool ChaletJsonParser::parseRoot(const Json& inNode) const
{
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			std::string val;
			if (valueMatchesSearchKeyPattern(val, value, key, Keys::SearchPaths, status))
				m_centralState.workspace.addSearchPath(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, Keys::SearchPaths, status))
				m_centralState.workspace.addSearchPaths(std::move(val));
			else if (isInvalid(status))
				return false;
		}
	}

	return true;
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
			const auto& [it, _] = m_abstractSourceTarget.emplace(name, std::make_unique<SourceTarget>(m_state));
			auto& abstract = it->second;
			if (!parseSourceTarget(*abstract, abstractJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' abstract project.", m_chaletJson.filename(), name);
				return false;
			}
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
		if (type == BuildTargetType::Project)
		{
			std::string extends{ "*" };
			if (targetJson.is_object())
			{
				m_chaletJson.assignFromKey(extends, targetJson, "extends");
			}

			if (m_abstractSourceTarget.find(extends) != m_abstractSourceTarget.end())
			{
				target = std::make_unique<SourceTarget>(static_cast<const SourceTarget&>(*m_abstractSourceTarget.at(extends))); // Note: copy ctor
			}
			else
			{
				if (!String::equals('*', extends))
				{
					Diagnostic::error("{}: project template '{}' is base of project '{}', but doesn't exist.", m_chaletJson.filename(), extends, name);
					return false;
				}

				target = IBuildTarget::make(type, m_state);
			}
		}
		else
		{
			target = IBuildTarget::make(type, m_state);
		}
		target->setName(name);

		auto conditionResult = parseTargetCondition(*target, targetJson);
		if (!conditionResult.has_value())
			return false;

		if (!(*conditionResult))
			continue; // true to skip project

		if (target->isScript())
		{
			// A script could be only for a specific platform
			if (!parseScriptTarget(static_cast<ScriptBuildTarget&>(*target), targetJson))
				return false;
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
	auto conditionResult = parseTargetCondition(outTarget, inNode);
	if (!conditionResult.has_value())
		return false;

	if (!(*conditionResult))
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
						else if (isInvalid(s))
							return false;
					}
					else if (v.is_array())
					{
						StringList val;
						if (valueMatchesSearchKeyPattern(val, v, k, "include", s))
							outTarget.addFiles(std::move(val));
						else if (isUnread(s) && valueMatchesSearchKeyPattern(val, v, k, "exclude", s))
							outTarget.addFileExcludes(std::move(val));
						else if (isInvalid(s))
							return false;
					}
				}
			}
		}
		else if (value.is_string())
		{
			std::string val;
			if (valueMatchesSearchKeyPattern(val, value, key, "outputDescription", status))
				outTarget.setOutputDescription(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "files", status))
				outTarget.addFile(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "configureFiles", status))
				outTarget.addConfigureFile(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "language", status))
				outTarget.setLanguage(value.get<std::string>());
			else if (isUnread(status) && String::equals("kind", key))
				outTarget.setKind(value.get<std::string>());
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "files", status))
				outTarget.addFiles(std::move(val));
			else if (valueMatchesSearchKeyPattern(val, value, key, "configureFiles", status))
				outTarget.addConfigureFiles(std::move(val));
			else if (isInvalid(status))
				return false;
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

	{
		const auto metadata{ "metadata" };
		if (inNode.contains(metadata))
		{
			const Json& node = inNode.at(metadata);
			if (!parseSourceTargetMetadata(outTarget, node))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::parseScriptTarget(ScriptBuildTarget& outTarget, const Json& inNode) const
{
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "outputDescription", status))
				outTarget.setOutputDescription(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "arguments", status))
				outTarget.addArgument(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "arguments", status))
				outTarget.addArguments(std::move(val));
			else if (isInvalid(status))
				return false;
		}
	}

	if (!parseRunTargetProperties(outTarget, inNode))
		return false;

	return valid;
}

/*****************************************************************************/
bool ChaletJsonParser::parseSubChaletTarget(SubChaletTarget& outTarget, const Json& inNode) const
{
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "outputDescription", status))
				outTarget.setOutputDescription(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "buildFile", status))
				outTarget.setBuildFile(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_boolean())
		{
			bool val = false;
			if (valueMatchesSearchKeyPattern(val, value, key, "recheck", status))
				outTarget.setRecheck(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "rebuild", status))
				outTarget.setRebuild(val);
			else if (isInvalid(status))
				return false;
		}
	}

	return valid;
}

/*****************************************************************************/
bool ChaletJsonParser::parseCMakeTarget(CMakeTarget& outTarget, const Json& inNode) const
{
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "outputDescription", status))
				outTarget.setOutputDescription(std::move(val));
			else if (isUnread(status) && String::equals("buildFile", key))
				outTarget.setBuildFile(value.get<std::string>());
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "toolset", status))
				outTarget.setToolset(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "runExecutable", status))
				outTarget.setRunExecutable(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "defines", status))
				outTarget.addDefine(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "targets", status))
				outTarget.addTarget(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "defines", status))
				outTarget.addDefines(std::move(val));
			else if (valueMatchesSearchKeyPattern(val, value, key, "targetNames", status))
				outTarget.addTargets(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_boolean())
		{
			bool val = false;
			if (valueMatchesSearchKeyPattern(val, value, key, "recheck", status))
				outTarget.setRecheck(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "rebuild", status))
				outTarget.setRebuild(val);
			else if (isInvalid(status))
				return false;
		}
	}

	if (!parseRunTargetProperties(outTarget, inNode))
		return false;

	return valid;
}

/*****************************************************************************/
bool ChaletJsonParser::parseProcessTarget(ProcessBuildTarget& outTarget, const Json& inNode) const
{
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "outputDescription", status))
				outTarget.setOutputDescription(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "arguments", status))
				outTarget.addArgument(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "arguments", status))
				outTarget.addArguments(std::move(val));
			else if (isInvalid(status))
				return false;
		}
	}

	return valid;
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
			{
				if (outTarget.name() == m_state.inputs.runTarget())
				{
					if (!m_state.inputs.runArguments().has_value())
						m_state.inputs.setRunArguments(std::move(val));
				}
			}
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "copyFilesOnRun", status))
				outTarget.addCopyFilesOnRun(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_string())
		{
			std::string val;
			if (valueMatchesSearchKeyPattern(val, value, key, "defaultRunArguments", status))
			{
				if (outTarget.name() == m_state.inputs.runTarget())
				{
					if (!m_state.inputs.runArguments().has_value())
						m_state.inputs.setRunArguments(std::move(val));
				}
			}
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "copyFilesOnRun", status))
				outTarget.addCopyFileOnRun(std::move(val));
			else if (isInvalid(status))
				return false;
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "precompiledHeader", status))
				outTarget.setPrecompiledHeader(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "cppStandard", status))
				outTarget.setCppStandard(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "cStandard", status))
				outTarget.setCStandard(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "warningsPreset", status))
				outTarget.setWarningPreset(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "warnings", status))
				outTarget.addWarning(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "linkerScript", status))
				outTarget.setLinkerScript(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "inputCharset", status))
				outTarget.setInputCharset(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "executionCharset", status))
				outTarget.setExecutionCharset(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "buildSuffix", status))
				outTarget.setBuildSuffix(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "positionIndependentCode", status))
				outTarget.setPicType(std::move(val));
			//
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "compileOptions", status))
				outTarget.addCompileOption(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "linkerOptions", status))
				outTarget.addLinkerOption(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "defines", status))
				outTarget.addDefine(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "links", status))
				outTarget.addLink(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "staticLinks", status))
				outTarget.addStaticLink(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "libDirs", status))
				outTarget.addLibDir(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "includeDirs", status))
				outTarget.addIncludeDir(std::move(val));
#if defined(CHALET_MACOS)
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "macosFrameworkPaths", status))
				outTarget.addMacosFrameworkPath(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "macosFrameworks", status))
				outTarget.addMacosFramework(std::move(val));
#endif
			else if (isInvalid(status))
				return false;
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "runtimeTypeInformation", status))
				outTarget.setRuntimeTypeInformation(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "positionIndependentCode", status))
				outTarget.setPicType(val);
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "staticRuntimeLibrary", status))
				outTarget.setStaticRuntimeLibrary(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "fastMath", status))
				outTarget.setFastMath(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "mingwUnixSharedLibraryNamingConvention", status))
				outTarget.setMinGWUnixSharedLibraryNamingConvention(val);
			// else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "windowsOutputDef", status))
			// 	outTarget.setWindowsOutputDef(val);
			else if (isInvalid(status))
				return false;

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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "compileOptions", status))
				outTarget.addCompileOptions(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "linkerOptions", status))
				outTarget.addLinkerOptions(std::move(val));
#if defined(CHALET_MACOS)
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "macosFrameworkPaths", status))
				outTarget.addMacosFrameworkPaths(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "macosFrameworks", status))
				outTarget.addMacosFrameworks(std::move(val));
#endif
			else if (isInvalid(status))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::parseSourceTargetMetadata(SourceTarget& outTarget, const Json& inNode) const
{
	Shared<TargetMetadata> metadata;
	if (outTarget.hasMetadata())
		metadata = std::make_shared<TargetMetadata>(outTarget.metadata());
	else
		metadata = std::make_shared<TargetMetadata>();

	bool hasMetadata = false;
	for (const auto& [key, value] : inNode.items())
	{
		if (value.is_string())
		{
			hasMetadata = true;

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

	if (hasMetadata)
	{
		outTarget.setMetadata(std::move(metadata));
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::parseDistribution(const Json& inNode) const
{
	if (!inNode.contains(Keys::Distribution))
		return true;

	const Json& distributionJson = inNode.at(Keys::Distribution);
	if (!distributionJson.is_object() || distributionJson.size() == 0)
	{
		Diagnostic::error("{}: '{}' must contain at least one bundle or script.", m_chaletJson.filename(), Keys::Distribution);
		return false;
	}

	for (auto& [name, targetJson] : distributionJson.items())
	{
		if (!targetJson.is_object())
		{
			Diagnostic::error("{}: distribution bundle '{}' must be an object.", m_chaletJson.filename(), name);
			return false;
		}

		DistTargetType type = DistTargetType::DistributionBundle;
		if (std::string val; m_chaletJson.assignFromKey(val, targetJson, "kind"))
		{
			if (String::equals("script", val))
			{
				type = DistTargetType::Script;
			}
			else if (String::equals("process", val))
			{
				type = DistTargetType::Process;
			}
			else if (String::equals("archive", val))
			{
				type = DistTargetType::BundleArchive;
			}
			else if (String::equals("macosDiskImage", val))
			{
#if defined(CHALET_MACOS)
				type = DistTargetType::MacosDiskImage;
#else
				continue;
#endif
			}
			else if (String::equals("bundle", val))
			{}
			else
			{
				Diagnostic::error("{}: Found unrecognized distribution kind of '{}'", m_chaletJson.filename(), val);
				return false;
			}
		}
		else
		{
			Diagnostic::error("{}: Found unrecognized distribution of '{}'", m_chaletJson.filename(), name);
			return false;
		}

		DistTarget target = IDistTarget::make(type, m_state);
		target->setName(name);

		auto conditionResult = parseTargetCondition(*target, targetJson);
		if (!conditionResult.has_value())
			return false;

		if (!(*conditionResult))
			continue; // skip target

		if (target->isScript())
		{
			if (!parseDistributionScript(static_cast<ScriptDistTarget&>(*target), targetJson))
				return false;
		}
		else if (target->isProcess())
		{
			if (!parseDistributionProcess(static_cast<ProcessDistTarget&>(*target), targetJson))
				return false;
		}
		else if (target->isArchive())
		{
			if (!parseDistributionArchive(static_cast<BundleArchiveTarget&>(*target), targetJson))
				return false;
		}
		else if (target->isDistributionBundle())
		{
			if (!parseDistributionBundle(static_cast<BundleTarget&>(*target), targetJson, inNode))
				return false;
		}
		else if (target->isMacosDiskImage())
		{
			if (!parseMacosDiskImage(static_cast<MacosDiskImageTarget&>(*target), targetJson))
				return false;
		}

		if (!target->includeInDistribution())
			continue;

		m_state.distribution.emplace_back(std::move(target));
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::parseDistributionScript(ScriptDistTarget& outTarget, const Json& inNode) const
{
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "outputDescription", status))
				outTarget.setOutputDescription(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "arguments", status))
				outTarget.addArgument(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "arguments", status))
				outTarget.addArguments(std::move(val));
			else if (isInvalid(status))
				return false;
		}
	}

	return valid;
}

/*****************************************************************************/
bool ChaletJsonParser::parseDistributionProcess(ProcessDistTarget& outTarget, const Json& inNode) const
{
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "outputDescription", status))
				outTarget.setOutputDescription(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "arguments", status))
				outTarget.addArgument(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "arguments", status))
				outTarget.addArguments(std::move(val));
			else if (isInvalid(status))
				return false;
		}
	}

	return valid;
}

/*****************************************************************************/
bool ChaletJsonParser::parseDistributionArchive(BundleArchiveTarget& outTarget, const Json& inNode) const
{
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			std::string val;
			if (valueMatchesSearchKeyPattern(val, value, key, "outputDescription", status))
				outTarget.setOutputDescription(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "include", status))
				outTarget.addInclude(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "format", status))
				outTarget.setFormat(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "include", status))
				outTarget.addIncludes(std::move(val));
			else if (isInvalid(status))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::parseDistributionBundle(BundleTarget& outTarget, const Json& inNode, const Json& inRoot) const
{
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			if (String::equals("buildTargets", key))
				outTarget.addBuildTarget(value.get<std::string>());
			else if (String::equals("outputDescription", key))
				outTarget.setOutputDescription(value.get<std::string>());
			else if (String::equals("subdirectory", key))
				outTarget.setSubdirectory(value.get<std::string>());
			else if (String::equals("mainExecutable", key))
				outTarget.setMainExecutable(value.get<std::string>());
			else if (String::equals("include", key))
				outTarget.addInclude(value.get<std::string>());
			else if (String::equals("exclude", key))
				outTarget.addExclude(value.get<std::string>());
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "buildTargets", status))
				outTarget.addBuildTargets(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "include", status))
				outTarget.addIncludes(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "exclude", status))
				outTarget.addExcludes(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_boolean())
		{
			if (String::equals("includeDependentSharedLibraries", key))
				outTarget.setIncludeDependentSharedLibraries(value.get<bool>());
		}
		else if (value.is_object())
		{
			if (String::equals("linuxDesktopEntry", key))
			{
#if defined(CHALET_LINUX)
				for (const auto& [k, v] : value.items())
				{
					if (v.is_string())
					{
						if (String::equals("template", k))
							outTarget.setLinuxDesktopEntryTemplate(v.get<std::string>());
						else if (String::equals("icon", k))
							outTarget.setLinuxDesktopEntryIcon(v.get<std::string>());
					}
				}
#endif
			}
			else if (String::equals("macosBundle", key))
			{
#if defined(CHALET_MACOS)
				outTarget.setMacosBundleName(outTarget.name());

				for (const auto& [k, v] : value.items())
				{
					if (v.is_string())
					{
						if (String::equals("type", k))
							outTarget.setMacosBundleType(v.get<std::string>());
						else if (String::equals("icon", k))
							outTarget.setMacosBundleIcon(v.get<std::string>());
						else if (String::equals("infoPropertyList", k))
							outTarget.setMacosBundleInfoPropertyList(v.get<std::string>());
					}
					else if (v.is_object())
					{
						if (String::equals("infoPropertyList", k))
						{
							outTarget.setMacosBundleInfoPropertyListContent(v.dump());
						}
					}
				}
#endif
			}
		}
	}

	if (outTarget.hasAllBuildTargets())
	{
		if (inRoot.contains(Keys::Targets))
		{
			const Json& targetsJson = inRoot.at(Keys::Targets);
			if (targetsJson.is_object())
			{
				const StringList validKinds{ "executable", "sharedLibrary", "staticLibrary" };
				StringList targetList;
				for (auto& [name, targetJson] : targetsJson.items())
				{
					if (targetJson.is_object() && targetJson.contains(Keys::Kind))
					{
						const Json& targetKind = targetJson.at(Keys::Kind);
						if (targetKind.is_string())
						{
							auto kind = targetKind.get<std::string>();
							if (String::contains(validKinds, kind))
							{
								targetList.push_back(name);
							}
						}
					}
				}

				if (!targetList.empty())
				{
					outTarget.addBuildTargets(std::move(targetList));
				}
			}
		}
	}
	else if (!outTarget.buildTargets().empty())
	{
		StringList targets;
		const auto& buildTargets = outTarget.buildTargets();
		if (inRoot.contains(Keys::Targets))
		{
			const Json& targetsJson = inRoot.at(Keys::Targets);
			if (targetsJson.is_object())
			{
				for (auto& [name, _] : targetsJson.items())
				{
					targets.push_back(name);
				}
			}
		}

		if (targets.empty())
			return false;

		for (auto& target : buildTargets)
		{
			if (!List::contains(targets, target))
			{
				Diagnostic::error("{}: Distribution bundle '{}' contains a build target that was not found: '{}'", m_chaletJson.filename(), outTarget.name(), target);
				return false;
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::parseMacosDiskImage(MacosDiskImageTarget& outTarget, const Json& inNode) const
{
	for (const auto& [key, value] : inNode.items())
	{
		if (value.is_string())
		{
			if (String::equals("outputDescription", key))
				outTarget.setOutputDescription(value.get<std::string>());
			else if (String::equals("background", key))
				outTarget.setBackground1x(value.get<std::string>());
		}
		else if (value.is_boolean())
		{
			if (String::equals("pathbarVisible", key))
				outTarget.setPathbarVisible(value.get<bool>());
		}
		else if (value.is_number())
		{
			if (String::equals("iconSize", key))
				outTarget.setIconSize(static_cast<ushort>(value.get<int>()));
			if (String::equals("textSize", key))
				outTarget.setTextSize(static_cast<ushort>(value.get<int>()));
		}
		else if (value.is_object())
		{
			if (String::equals("background", key))
			{
				for (const auto& [k, v] : value.items())
				{
					if (v.is_string())
					{
						if (String::equals("1x", k))
							outTarget.setBackground1x(v.get<std::string>());
						else if (String::equals("2x", k))
							outTarget.setBackground2x(v.get<std::string>());
					}
				}
			}
			else if (String::equals("size", key))
			{
				int width = 0;
				int height = 0;
				for (const auto& [k, v] : value.items())
				{
					if (v.is_number())
					{
						if (String::equals("width", k))
							width = v.get<int>();
						else if (String::equals("height", k))
							height = v.get<int>();
					}
				}
				if (width > 0 && height > 0)
				{
					outTarget.setSize(static_cast<ushort>(width), static_cast<ushort>(height));
				}
			}
			else if (String::equals("positions", key))
			{
				for (const auto& [name, posJson] : value.items())
				{
					if (posJson.is_object())
					{
						int posX = 0;
						int posY = 0;
						for (const auto& [k, v] : posJson.items())
						{
							if (v.is_number())
							{
								if (String::equals("x", k))
									posX = v.get<int>();
								else if (String::equals("y", k))
									posY = v.get<int>();
							}
						}

						outTarget.addPosition(name, static_cast<short>(posX), static_cast<short>(posY));
					}
				}
			}
		}
	}

	return true;
}

/*****************************************************************************/
std::optional<bool> ChaletJsonParser::parseTargetCondition(IBuildTarget& outTarget, const Json& inNode) const
{
	if (std::string val; m_chaletJson.assignFromKey(val, inNode, "condition"))
	{
		auto res = conditionIsValid(outTarget, val);
		if (res.has_value())
			outTarget.setIncludeInBuild(*res);
		else
			return std::nullopt;
	}

	return outTarget.includeInBuild();
}

/*****************************************************************************/
std::optional<bool> ChaletJsonParser::parseTargetCondition(IDistTarget& outTarget, const Json& inNode) const
{
	if (std::string val; m_chaletJson.assignFromKey(val, inNode, "condition"))
	{
		auto res = conditionIsValid(val);
		if (res.has_value())
			outTarget.setIncludeInDistribution(*res);
		else
			return std::nullopt;
	}

	return outTarget.includeInDistribution();
}

/*****************************************************************************/
std::optional<bool> ChaletJsonParser::conditionIsValid(IBuildTarget& outTarget, const std::string& inContent) const
{
	if (!matchConditionVariables(inContent, [this, &inContent, &outTarget](const std::string& key, const std::string& value, bool negate) {
			auto res = checkConditionVariable(outTarget, inContent, key, value, negate);
			return res == ConditionResult::Pass;
		}))
	{
		if (m_lastOp == ConditionOp::InvalidOr)
		{
			Diagnostic::error("Syntax for AND '+', OR '|' are mutually exclusive. Both found in: {}", inContent);
			return std::nullopt;
		}

		return false;
	}

	return true;
}

/*****************************************************************************/
std::optional<bool> ChaletJsonParser::conditionIsValid(const std::string& inContent) const
{
	if (!matchConditionVariables(inContent, [this, &inContent](const std::string& key, const std::string& value, bool negate) {
			auto res = checkConditionVariable(inContent, key, value, negate);
			return res == ConditionResult::Pass;
		}))
	{
		if (m_lastOp == ConditionOp::InvalidOr)
		{
			Diagnostic::error("Syntax for AND '+', OR '|' are mutually exclusive. Both found in: {}", inContent);
			return std::nullopt;
		}

		return false;
	}

	return true;
}

/*****************************************************************************/
ChaletJsonParser::ConditionResult ChaletJsonParser::checkConditionVariable(IBuildTarget& outTarget, const std::string& inString, const std::string& key, const std::string& value, bool negate) const
{
	if (key.empty())
	{
		if (String::equals("runTarget", value))
		{
			const bool routeWillRun = m_state.inputs.routeWillRun();
			const auto& runTarget = m_state.inputs.runTarget();

			if (!routeWillRun || runTarget.empty())
				return ConditionResult::Fail;

			if (negate)
			{
				if (String::equals(runTarget, outTarget.name()))
					return ConditionResult::Fail;
			}
			else
			{
				if (!String::equals(runTarget, outTarget.name()))
					return ConditionResult::Fail;
			}

			return ConditionResult::Pass;
		}
	}
	else if (String::equals("options", key))
	{
		if (String::equals("runTarget", value))
		{
			const bool routeWillRun = m_state.inputs.routeWillRun();
			const auto& runTarget = m_state.inputs.runTarget();

			if (!routeWillRun || runTarget.empty())
				return ConditionResult::Fail;

			if (negate)
			{
				if (String::equals(runTarget, outTarget.name()))
					return ConditionResult::Fail;
			}
			else
			{
				if (!String::equals(runTarget, outTarget.name()))
					return ConditionResult::Fail;
			}

			return ConditionResult::Pass;
		}
		else
		{
			Diagnostic::error("Invalid condition '{}:{}' found in: {}", key, value, inString);
			return ConditionResult::Invalid;
		}
	}

	return checkConditionVariable(inString, key, value, negate);
}

/*****************************************************************************/
ChaletJsonParser::ConditionResult ChaletJsonParser::checkConditionVariable(const std::string& inString, const std::string& key, const std::string& value, bool negate) const
{
	// LOG("  ", key, value, negate);

	if (key.empty())
	{
		// :value or :{value} syntax
		if (String::equals("debug", value))
		{
			if (negate)
			{
				if (m_state.configuration.debugSymbols())
					return ConditionResult::Fail;
			}
			else
			{
				if (!m_state.configuration.debugSymbols())
					return ConditionResult::Fail;
			}
		}
		else if (String::equals(kValidPlatforms, value))
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
	else if (String::equals("toolchain", key))
	{
		const auto& triple = m_state.info.targetArchitectureTriple();
		const auto& toolchainName = m_state.inputs.toolchainPreferenceName();

		if (negate)
		{
			if (String::contains(value, triple) || String::contains(value, toolchainName))
				return ConditionResult::Fail;
		}
		else
		{
			if (!String::contains(value, triple) && !String::contains(value, toolchainName))
				return ConditionResult::Fail;
		}
	}
	else if (String::equals("architecture", key))
	{
		const auto& arch = m_state.info.targetArchitectureString();
		if (negate)
		{
			if (String::equals(value, arch))
				return ConditionResult::Fail;
		}
		else
		{
			if (!String::equals(value, arch))
				return ConditionResult::Fail;
		}
	}
	else if (String::equals("configuration", key))
	{
		if (String::equals("debugSymbols", value))
		{
			if (negate)
			{
				if (m_state.configuration.debugSymbols())
					return ConditionResult::Fail;
			}
			else
			{
				if (!m_state.configuration.debugSymbols())
					return ConditionResult::Fail;
			}
		}
		else if (String::equals("enableProfiling", value))
		{
			if (negate)
			{
				if (m_state.configuration.enableProfiling())
					return ConditionResult::Fail;
			}
			else
			{
				if (!m_state.configuration.enableProfiling())
					return ConditionResult::Fail;
			}
		}
		else if (String::equals("interproceduralOptimization", value))
		{
			if (negate)
			{
				if (m_state.configuration.interproceduralOptimization())
					return ConditionResult::Fail;
			}
			else
			{
				if (!m_state.configuration.interproceduralOptimization())
					return ConditionResult::Fail;
			}
		}
		else
		{
			Diagnostic::error("Invalid condition '{}:{}' found in: {}", key, value, inString);
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

/*****************************************************************************/
bool ChaletJsonParser::matchConditionVariables(const std::string& inText, const std::function<bool(const std::string&, const std::string&, bool)>& onMatch) const
{
	auto squareBracketBegin = inText.find('[');
	if (squareBracketBegin == std::string::npos)
		return true;

	auto squareBracketEnd = inText.find(']', squareBracketBegin);
	if (squareBracketEnd == std::string::npos)
		return true;

	// LOG("evaluating:", inText);

	char op;
	auto raw = inText.substr(squareBracketBegin + 1, (squareBracketEnd - squareBracketBegin) - 1);
	if (raw.find('|') != std::string::npos)
	{
		m_lastOp = ConditionOp::Or;
		op = '|';
	}
	else
	{
		m_lastOp = ConditionOp::And;
		op = '+';
	}

	bool isOr = m_lastOp == ConditionOp::Or;
	if (isOr && raw.find('+') != std::string::npos)
	{
		m_lastOp = ConditionOp::InvalidOr;
		return false;
	}

	bool result = !isOr;
	auto split = String::split(raw, op);
	for (auto& prop : split)
	{
		auto propSplit = String::split(prop, ':');
		auto& key = propSplit.front();
		if (key.find_first_of("!{},") != std::string::npos)
		{
			result = false;
			break;
		}

		if (propSplit.size() > 1)
		{
			auto& inner = propSplit.at(1);
			if (inner.front() == '{' && inner.back() == '}')
			{
				inner = inner.substr(1);
				inner.pop_back();
			}

			if (String::contains(',', inner))
			{
				bool valid = false;
				auto innerSplit = String::split(inner, ',');
				for (auto& v : innerSplit)
				{
					bool negate = false;
					if (v.front() == '!')
					{
						v = v.substr(1);
						negate = true;
					}

					if (v.find_first_of("!{},") == std::string::npos)
					{
						valid |= onMatch(key, v, negate);
					}
					else
					{
						valid = false;
						break;
					}
				}
				if (isOr)
					result |= valid;
				else
					result &= valid;
			}
			else
			{
				bool negate = false;
				if (inner.front() == '!')
				{
					inner = inner.substr(1);
					negate = true;
				}

				if (inner.find_first_of("!{},") == std::string::npos)
				{
					if (isOr)
						result |= onMatch(key, inner, negate);
					else
						result &= onMatch(key, inner, negate);
				}
				else
				{
					result = false;
					break;
				}
			}
		}
		else
		{
			bool negate = false;
			if (key.front() == '!')
			{
				key = key.substr(1);
				negate = true;
			}

			if (isOr)
				result |= onMatch(key, std::string{}, negate);
			else
				result &= onMatch(key, std::string{}, negate);
		}
	}

	return result;
}
}
