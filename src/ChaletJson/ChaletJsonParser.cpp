/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

// Note: mind the order
#include "State/BuildState.hpp"
#include "Json/JsonFile.hpp"
//
#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "ChaletJson/ChaletJsonParser.hpp"
#include "ChaletJson/ChaletJsonSchema.hpp"
#include "Compile/ToolchainTypes.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Platform/Platform.hpp"
#include "Process/Environment.hpp"
#include "State/BuildInfo.hpp"
#include "State/CentralState.hpp"
#include "State/Dependency/ExternalDependencyType.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Package/SourcePackage.hpp"
#include "State/PackageManager.hpp"
#include "State/TargetMetadata.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonKeys.hpp"
#include "Json/JsonValues.hpp"

#include "State/Target/CMakeTarget.hpp"
#include "State/Target/MesonTarget.hpp"
#include "State/Target/ProcessBuildTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "State/Target/ValidationBuildTarget.hpp"

#include "State/Distribution/BundleArchiveTarget.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/MacosDiskImageTarget.hpp"
#include "State/Distribution/ProcessDistTarget.hpp"
#include "State/Distribution/ScriptDistTarget.hpp"
#include "State/Distribution/ValidationDistTarget.hpp"

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
constexpr const char kCondition[] = "condition";
}

/*****************************************************************************/
ChaletJsonParser::ChaletJsonParser(BuildState& inState) :
	m_chaletJson(inState.getCentralState().chaletJson()),
	m_state(inState),
	kValidPlatforms(Platform::validPlatforms())
{
	Platform::assignPlatform(m_state.inputs, m_platform, m_notPlatforms);
	m_isWebPlatform = String::equals("web", m_platform);
}

/*****************************************************************************/
ChaletJsonParser::~ChaletJsonParser() = default;

/*****************************************************************************/
bool ChaletJsonParser::serialize()
{
	// Timer timer;
	// Diagnostic::infoEllipsis("Reading Build File [{}]", m_chaletJson.filename());

	const Json& jRoot = m_chaletJson.root;
	if (!serializeFromJsonRoot(jRoot))
	{
		Diagnostic::error("{}: There was an error parsing the file.", m_chaletJson.filename());
		return false;
	}

	if (!validBuildRequested())
		return false;

	if (m_state.inputs.route().willRun())
	{
		// Note: done after parsing
		auto runTarget = getValidRunTargetFromInput();
		if (runTarget.empty())
			return false;

		// do after run target is validated
		auto& centralState = m_state.getCentralState();
		auto& runArguments = centralState.getRunTargetArguments(runTarget);
		bool hasRunTargetsFromInput = m_state.inputs.runArguments().has_value();
		if (runArguments.has_value() && !hasRunTargetsFromInput)
			m_state.inputs.setRunArguments(*runArguments);

		if (hasRunTargetsFromInput)
		{
			// Update the inputs instance in central state
			centralState.inputs().setRunArguments(*m_state.inputs.runArguments());
			centralState.setRunArguments(runTarget, StringList(*m_state.inputs.runArguments()));
		}
	}

	// Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::readPackagesIfAvailable(const std::string& inFilename, const std::string& inRoot)
{
	JsonFile buildFile(inFilename);
	if (!buildFile.load())
		return false;

	if (!parsePackage(buildFile.root, inRoot))
		return false;

	return true;
}

/*****************************************************************************/
const StringList& ChaletJsonParser::getBuildTargets() const
{
	if (m_buildTargets.empty())
	{
		m_buildTargets = m_state.inputs.getBuildTargets();
		if (m_buildTargets.empty())
			m_buildTargets.emplace_back(Values::All);
	}
	return m_buildTargets;
}

/*****************************************************************************/
bool ChaletJsonParser::serializeFromJsonRoot(const Json& inJson)
{
	if (!parseRoot(inJson))
		return false;

	if (!m_state.getCentralState().inputs().route().isConfigure())
	{
		if (!parsePlatformRequires(inJson))
			return false;

		if (!parseDistribution(inJson))
			return false;
	}

	if (!parseTargets(inJson))
		return false;

	if (!parsePackage(inJson))
		return false;

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::validBuildRequested() const
{
	StringList targetNamesLowerCase;

	i32 count = 0;
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

			auto nameLowerCase = String::toLowerCase(project.name());
			if (List::contains(targetNamesLowerCase, nameLowerCase))
			{
				Diagnostic::error("{}: Targets must have unique case-insensitive names, but '{}' matched a target that was previously declared.", m_chaletJson.filename(), project.name());
				return false;
			}

			targetNamesLowerCase.emplace_back(std::move(nameLowerCase));
		}
	}

	if (count == 0)
	{
		const auto& buildConfiguration = m_state.configuration.name();
		Diagnostic::error("{}: No valid targets to build for '{}' configuration. Check usage of '_+' property", m_chaletJson.filename(), buildConfiguration, kCondition);
		return false;
	}

	return true;
}

/*****************************************************************************/
std::string ChaletJsonParser::getValidRunTargetFromInput() const
{
	auto target = m_state.getFirstValidRunTarget();
	if (target != nullptr)
		return target->name();

	auto& filename = m_chaletJson.filename();
	auto& lastTarget = m_state.inputs.lastTarget();

	if (String::contains(',', lastTarget))
		Diagnostic::error("{}: '{}' either does not contain an executable target, or are excluded based on property conditions.", filename, lastTarget);
	else
		Diagnostic::error("{}: '{}' is either not an executable target, or is excluded based on a property condition.", filename, lastTarget);

	return std::string();
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
				m_state.workspace.addSearchPath(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, Keys::PackagePaths, status))
				m_state.packages.addPackagePath(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, Keys::SearchPaths, status))
				m_state.workspace.addSearchPaths(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, Keys::PackagePaths, status))
				m_state.packages.addPackagePaths(std::move(val));
			else if (isInvalid(status))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::parsePlatformRequires(const Json& inNode) const
{
	if (!inNode.contains(Keys::PlatformRequires))
		return true;

	const Json& platformRequires = inNode[Keys::PlatformRequires];
	for (const auto& [key, value] : platformRequires.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			std::string val;
#if defined(CHALET_WIN32)
			if (valueMatchesSearchKeyPattern(val, value, key, Keys::ReqWindowsMSYS2, status))
				m_state.info.addRequiredPlatformDependency(Keys::ReqWindowsMSYS2, String::split(val));
#elif defined(CHALET_MACOS)
			if (valueMatchesSearchKeyPattern(val, value, key, Keys::ReqMacOSMacPorts, status))
				m_state.info.addRequiredPlatformDependency(Keys::ReqMacOSMacPorts, String::split(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, Keys::ReqMacOSHomebrew, status))
				m_state.info.addRequiredPlatformDependency(Keys::ReqMacOSHomebrew, String::split(val));
#else
			if (valueMatchesSearchKeyPattern(val, value, key, Keys::ReqUbuntuSystem, status))
				m_state.info.addRequiredPlatformDependency(Keys::ReqUbuntuSystem, String::split(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, Keys::ReqDebianSystem, status))
				m_state.info.addRequiredPlatformDependency(Keys::ReqDebianSystem, String::split(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, Keys::ReqArchLinuxSystem, status))
				m_state.info.addRequiredPlatformDependency(Keys::ReqArchLinuxSystem, String::split(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, Keys::ReqManjaroSystem, status))
				m_state.info.addRequiredPlatformDependency(Keys::ReqManjaroSystem, String::split(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, Keys::ReqFedoraSystem, status))
				m_state.info.addRequiredPlatformDependency(Keys::ReqFedoraSystem, String::split(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, Keys::ReqRedHatSystem, status))
				m_state.info.addRequiredPlatformDependency(Keys::ReqRedHatSystem, String::split(val));
#endif
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
#if defined(CHALET_WIN32)
			if (valueMatchesSearchKeyPattern(val, value, key, "windows.msys2", status))
				m_state.info.addRequiredPlatformDependency("windows.msys2", std::move(val));
#elif defined(CHALET_MACOS)
			if (valueMatchesSearchKeyPattern(val, value, key, "macos.macports", status))
				m_state.info.addRequiredPlatformDependency("macos.macports", std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "macos.homebrew", status))
				m_state.info.addRequiredPlatformDependency("macos.homebrew", std::move(val));
#else
			if (valueMatchesSearchKeyPattern(val, value, key, "ubuntu.system", status))
				m_state.info.addRequiredPlatformDependency("ubuntu.system", std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "debian.system", status))
				m_state.info.addRequiredPlatformDependency("debian.system", std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "archlinux.system", status))
				m_state.info.addRequiredPlatformDependency("archlinux.system", std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "manjaro.system", status))
				m_state.info.addRequiredPlatformDependency("manjaro.system", std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "fedora.system", status))
				m_state.info.addRequiredPlatformDependency("fedora.system", std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "redhat.system", status))
				m_state.info.addRequiredPlatformDependency("redhat.system", std::move(val));
#endif

			else if (isInvalid(status))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::parsePackage(const Json& inNode, const std::string& inRoot) const
{
	if (!inNode.contains(Keys::Package))
		return true;

	const Json& packageRoot = inNode[Keys::Package];
	if (!packageRoot.is_object() || packageRoot.empty())
	{
		Diagnostic::error("{}: '{}' must contain at least one target.", m_chaletJson.filename(), Keys::Package);
		return false;
	}

	for (auto& [name, packageJson] : packageRoot.items())
	{
		if (!packageJson.is_object())
		{
			Diagnostic::error("{}: package '{}' must be an object.", m_chaletJson.filename(), name);
			return false;
		}

		auto package = std::make_shared<SourcePackage>(m_state);
		package->setName(name);
		package->setRoot(inRoot);

		if (!parsePackageTarget(*package, packageJson))
		{
			Diagnostic::error("{}: Error parsing the '{}' package.", m_chaletJson.filename(), name);
			return false;
		}

		m_state.packages.add(name, std::move(package));
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::parsePackageTarget(SourcePackage& outPackage, const Json& inNode) const
{
	auto& packageName = outPackage.name();
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			std::string val;
			if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "searchPaths", status))
				outPackage.addSearchPath(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "copyFilesOnRun", status))
				outPackage.addCopyFileOnRun(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "importPackages", status))
				m_state.packages.addPackageDependency(packageName, std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "searchPaths", status))
				outPackage.addSearchPaths(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "copyFilesOnRun", status))
				outPackage.addCopyFilesOnRun(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "importPackages", status))
				m_state.packages.addPackageDependencies(packageName, std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_object())
		{
			if (String::equals("settings:Cxx", key))
			{
				if (!parsePackageSettingsCxx(outPackage, value))
					return false;
			}
			else if (String::equals("settings", key))
			{
				for (const auto& [k, v] : value.items())
				{
					if (v.is_object() && String::equals("Cxx", k))
					{
						if (!parsePackageSettingsCxx(outPackage, v))
							return false;
					}
				}
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::parsePackageSettingsCxx(SourcePackage& outPackage, const Json& inNode) const
{
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			std::string val;
			if (valueMatchesSearchKeyPattern(val, value, key, "linkerOptions", status))
				outPackage.addLinkerOption(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "links", status))
				outPackage.addLink(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "staticLinks", status))
				outPackage.addStaticLink(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "libDirs", status))
				outPackage.addLibDir(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "includeDirs", status))
				outPackage.addIncludeDir(std::move(val));
#if defined(CHALET_MACOS)
			else if (!m_isWebPlatform && isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "appleFrameworkPaths", status))
				outPackage.addAppleFrameworkPath(std::move(val));
			else if (!m_isWebPlatform && isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "appleFrameworks", status))
				outPackage.addAppleFramework(std::move(val));
#endif
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_boolean())
		{
			return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "links", status))
				outPackage.addLinks(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "staticLinks", status))
				outPackage.addStaticLinks(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "libDirs", status))
				outPackage.addLibDirs(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "includeDirs", status))
				outPackage.addIncludeDirs(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "linkerOptions", status))
				outPackage.addLinkerOptions(std::move(val));
#if defined(CHALET_MACOS)
			else if (!m_isWebPlatform && isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "appleFrameworkPaths", status))
				outPackage.addAppleFrameworkPaths(std::move(val));
			else if (!m_isWebPlatform && isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "appleFrameworks", status))
				outPackage.addAppleFrameworks(std::move(val));
#endif
			else if (isInvalid(status))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool ChaletJsonParser::parseTargets(const Json& inNode)
{
	if (!inNode.contains(Keys::Targets))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_chaletJson.filename(), Keys::Targets);
		return false;
	}

	const Json& targets = inNode[Keys::Targets];
	if (!targets.is_object() || targets.empty())
	{
		Diagnostic::error("{}: '{}' must contain at least one target.", m_chaletJson.filename(), Keys::Targets);
		return false;
	}

	if (inNode.contains(Keys::Abstracts))
	{
		const Json& abstracts = inNode[Keys::Abstracts];
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
				Diagnostic::error("{}: Error parsing the '{}' abstract target.", m_chaletJson.filename(), name);
				return false;
			}
		}
		else
		{
			// not sure if this would actually get triggered?
			Diagnostic::error("{}: Abstract target '{}' already exists.", m_chaletJson.filename(), name);
			return false;
		}
	}

	if (m_abstractSourceTarget.find(Values::All) != m_abstractSourceTarget.end())
	{
		Diagnostic::error("{}: 'all' is a reserved build target name, and cannot be used inside 'abstracts'.", m_chaletJson.filename());
		return false;
	}

	StringList sourceTargets{
		"executable",
		"staticLibrary",
		"sharedLibrary",
	};

	for (auto& [name, targetJson] : targets.items())
	{
		if (String::equals(Values::All, name))
		{
			Diagnostic::error("{}: 'all' is a reserved build target name, and cannot be used inside 'targets'.", m_chaletJson.filename());
			return false;
		}

		if (!targetJson.is_object())
		{
			Diagnostic::error("{}: target '{}' must be an object.", m_chaletJson.filename(), name);
			return false;
		}

		BuildTargetType type = BuildTargetType::Unknown;
		if (std::string val; json::assign(val, targetJson, "kind"))
		{
			if (String::equals(sourceTargets, val))
			{
				type = BuildTargetType::Source;
			}
			else if (String::equals("chaletProject", val))
			{
				type = BuildTargetType::SubChalet;
			}
			else if (String::equals("cmakeProject", val))
			{
				type = BuildTargetType::CMake;
			}
			else if (String::equals("mesonProject", val))
			{
				type = BuildTargetType::Meson;
			}
			else if (String::equals("script", val))
			{
				type = BuildTargetType::Script;
			}
			else if (String::equals("process", val))
			{
				type = BuildTargetType::Process;
			}
			else if (String::equals("validation", val))
			{
				type = BuildTargetType::Validation;
			}
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
		if (type == BuildTargetType::Source)
		{
			std::string extends{ "*" };
			if (targetJson.is_object())
			{
				json::assign(extends, targetJson, "extends");
			}

			if (m_abstractSourceTarget.find(extends) != m_abstractSourceTarget.end())
			{
				target = std::make_unique<SourceTarget>(static_cast<const SourceTarget&>(*m_abstractSourceTarget.at(extends))); // Note: copy ctor
			}
			else
			{
				if (!String::equals('*', extends))
				{
					Diagnostic::error("{}: Build target '{}' extends '{}', but doesn't exist.", m_chaletJson.filename(), extends, name);
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

		if (target->isSubChalet())
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
		else if (target->isMeson())
		{
			if (!parseMesonTarget(static_cast<MesonTarget&>(*target), targetJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' target of type 'mesonTarget'.", m_chaletJson.filename(), name);
				return false;
			}
		}
		else if (target->isScript())
		{
			// A script could be only for a specific platform
			if (!parseScriptTarget(static_cast<ScriptBuildTarget&>(*target), targetJson))
				return false;
		}
		else if (target->isProcess())
		{
			if (!parseProcessTarget(static_cast<ProcessBuildTarget&>(*target), targetJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' target of type 'process'.", m_chaletJson.filename(), name);
				return false;
			}
		}
		else if (target->isValidation())
		{
			if (!parseValidationTarget(static_cast<ValidationBuildTarget&>(*target), targetJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' target of type 'process'.", m_chaletJson.filename(), name);
				return false;
			}
		}
		else
		{
			if (!parseSourceTarget(static_cast<SourceTarget&>(*target), targetJson))
			{
				Diagnostic::error("{}: Error parsing the '{}' build target.", m_chaletJson.filename(), name);
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "importPackages", status))
				outTarget.addImportPackage(std::move(val));
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "configureFiles", status))
				outTarget.addConfigureFiles(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "importPackages", status))
				outTarget.addImportPackages(std::move(val));
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
			const Json& jCompilerSettings = inNode[compilerSettings];
			if (jCompilerSettings.contains("Cxx"))
			{
				const Json& node = jCompilerSettings["Cxx"];
				if (!parseCompilerSettingsCxx(outTarget, node))
					return false;
			}
		}

		const auto compilerSettingsCpp = fmt::format("{}:Cxx", compilerSettings);
		if (inNode.contains(compilerSettingsCpp))
		{
			const Json& node = inNode[compilerSettingsCpp];
			if (!parseCompilerSettingsCxx(outTarget, node))
				return false;
		}
	}

	{
		const auto metadata{ "metadata" };
		if (inNode.contains(metadata))
		{
			const Json& node = inNode[metadata];
			if (!parseSourceTargetMetadata(outTarget, node))
				return false;
		}
	}

	return true;
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "targets", status))
				outTarget.addTarget(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "targets", status))
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "clean", status))
				outTarget.setClean(val);
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "targets", status))
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "clean", status))
				outTarget.setClean(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "install", status))
				outTarget.setInstall(val);
			else if (isInvalid(status))
				return false;
		}
	}

	if (!parseRunTargetProperties(outTarget, inNode))
		return false;

	return valid;
}

/*****************************************************************************/
bool ChaletJsonParser::parseMesonTarget(MesonTarget& outTarget, const Json& inNode) const
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "targets", status))
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "clean", status))
				outTarget.setClean(val);
			else if (isInvalid(status))
				return false;
		}
	}

	if (!parseRunTargetProperties(outTarget, inNode))
		return false;

	return valid;
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "dependsOn", status))
				outTarget.addDependsOn(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_boolean())
		{
			bool val = false;
			if (valueMatchesSearchKeyPattern(val, value, key, "dependsOnSelf", status))
				outTarget.setDependsOnSelf(val);
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "arguments", status))
				outTarget.addArguments(std::move(val));
			else if (valueMatchesSearchKeyPattern(val, value, key, "dependsOn", status))
				outTarget.addDependsOn(std::move(val));
			else if (isInvalid(status))
				return false;
		}
	}

	if (!parseRunTargetProperties(outTarget, inNode))
		return false;

	if (!valid)
	{
		// When a script has a "file" that is conditional to another platform
		outTarget.setIncludeInBuild(false);
	}

	return true;
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "dependsOn", status))
				outTarget.addDependsOn(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "arguments", status))
				outTarget.addArguments(std::move(val));
			else if (valueMatchesSearchKeyPattern(val, value, key, "dependsOn", status))
				outTarget.addDependsOn(std::move(val));
			else if (isInvalid(status))
				return false;
		}
	}

	return valid;
}

/*****************************************************************************/
bool ChaletJsonParser::parseValidationTarget(ValidationBuildTarget& outTarget, const Json& inNode) const
{
	bool hasSchema = false;
	bool hasFiles = false;
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			std::string val;
			if (valueMatchesSearchKeyPattern(val, value, key, "schema", status))
			{
				outTarget.setSchema(std::move(val));
				hasSchema = true;
			}
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "files", status))
			{
				outTarget.addFile(std::move(val));
				hasFiles = true;
			}
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "outputDescription", status))
				outTarget.setOutputDescription(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "files", status))
			{
				outTarget.addFiles(std::move(val));
				hasFiles = true;
			}
			else if (isInvalid(status))
				return false;
		}
	}

	return hasSchema && hasFiles;
}

/*****************************************************************************/
bool ChaletJsonParser::parseRunTargetProperties(IBuildTarget& outTarget, const Json& inNode) const
{
	StringList validNames{
		outTarget.name(),
		Values::All,
	};

	auto& centralState = m_state.getCentralState();
	bool getDefaultRunArguments = m_state.inputs.route().isExport() || m_state.inputs.route().willRun();
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_array())
		{
			StringList val;
			if (getDefaultRunArguments && valueMatchesSearchKeyPattern(val, value, key, "defaultRunArguments", status))
			{
				centralState.addRunArgumentsIfNew(outTarget.name(), std::move(val));
			}
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "copyFilesOnRun", status))
			{
				if (outTarget.isSources())
				{
					auto& project = static_cast<SourceTarget&>(outTarget);
					project.addCopyFilesOnRun(std::move(val));
				}
			}
			else if (isInvalid(status))
			{
				return false;
			}
		}
		else if (value.is_string())
		{
			std::string val;
			if (getDefaultRunArguments && valueMatchesSearchKeyPattern(val, value, key, "defaultRunArguments", status))
			{
				centralState.addRunArgumentsIfNew(outTarget.name(), centralState.getArgumentStringListFromString(val));
			}
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "copyFilesOnRun", status))
			{
				if (outTarget.isSources())
				{
					auto& project = static_cast<SourceTarget&>(outTarget);
					project.addCopyFileOnRun(std::move(val));
				}
			}
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "ccacheOptions", status))
				outTarget.addCcacheOption(std::move(val));
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
			else if (!m_isWebPlatform && isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "appleFrameworkPaths", status))
				outTarget.addAppleFrameworkPath(std::move(val));
			else if (!m_isWebPlatform && isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "appleFrameworks", status))
				outTarget.addAppleFramework(std::move(val));
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "unityBuild", status))
				outTarget.setUnityBuild(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "mingwUnixSharedLibraryNamingConvention", status))
				outTarget.setMinGWUnixSharedLibraryNamingConvention(val);
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "justMyCodeDebugging", status))
				outTarget.setJustMyCodeDebugging(val);
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "ccacheOptions", status))
				outTarget.addCcacheOptions(std::move(val));
#if defined(CHALET_MACOS)
			else if (!m_isWebPlatform && isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "appleFrameworkPaths", status))
				outTarget.addAppleFrameworkPaths(std::move(val));
			else if (!m_isWebPlatform && isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "appleFrameworks", status))
				outTarget.addAppleFrameworks(std::move(val));
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
	Ref<TargetMetadata> metadata;

	if (inNode.is_string())
	{
		const auto value = inNode.get<std::string>();
		if (String::equals("workspace", value))
		{
			metadata = std::make_shared<TargetMetadata>(m_state.workspace.metadata());
			outTarget.setMetadata(std::move(metadata));
			return true;
		}
		else
		{
			return false;
		}
	}

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

	const Json& distributionJson = inNode[Keys::Distribution];
	if (!distributionJson.is_object() || distributionJson.empty())
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

		DistTargetType type = DistTargetType::Unknown;
		if (std::string val; json::assign(val, targetJson, "kind"))
		{
			if (String::equals("bundle", val))
			{
				type = DistTargetType::DistributionBundle;
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
			else if (String::equals("script", val))
			{
				type = DistTargetType::Script;
			}
			else if (String::equals("process", val))
			{
				type = DistTargetType::Process;
			}
			else if (String::equals("validation", val))
			{
				type = DistTargetType::Validation;
			}
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

		if (target->isDistributionBundle())
		{
			if (!parseDistributionBundle(static_cast<BundleTarget&>(*target), targetJson, inNode))
				return false;
		}
		else if (target->isArchive())
		{
			if (!parseDistributionArchive(static_cast<BundleArchiveTarget&>(*target), targetJson))
				return false;
		}
		else if (target->isMacosDiskImage())
		{
			if (!parseMacosDiskImage(static_cast<MacosDiskImageTarget&>(*target), targetJson))
				return false;
		}
		else if (target->isScript())
		{
			if (!parseDistributionScript(static_cast<ScriptDistTarget&>(*target), targetJson))
				return false;
		}
		else if (target->isProcess())
		{
			if (!parseDistributionProcess(static_cast<ProcessDistTarget&>(*target), targetJson))
				return false;
		}
		else if (target->isValidation())
		{
			if (!parseDistributionValidation(static_cast<ValidationDistTarget&>(*target), targetJson))
				return false;
		}

		if (!target->includeInDistribution())
			continue;

		m_state.distribution.emplace_back(std::move(target));
	}

	return true;
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
#if defined(CHALET_MACOS)
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "macosNotarizationProfile", status))
				outTarget.setMacosNotarizationProfile(std::move(val));
#endif
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
		else if (value.is_object())
		{
			if (valueMatchesSearchKeyPattern(key, "include", status))
			{
				for (const auto& [k, v] : value.items())
				{
					if (v.is_string())
					{
						outTarget.addInclude(k, v.get<std::string>());
					}
				}
			}
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
			std::string val;
			if (valueMatchesSearchKeyPattern(val, value, key, "outputDescription", status))
				outTarget.setOutputDescription(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "buildTargets", status))
				outTarget.addBuildTarget(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "subdirectory", status))
				outTarget.setSubdirectory(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "mainExecutable", status))
				outTarget.setMainExecutable(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "include", status))
				outTarget.addInclude(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "exclude", status))
				outTarget.addExclude(std::move(val));
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
			if (valueMatchesSearchKeyPattern(key, "include", status))
			{
				for (const auto& [k, v] : value.items())
				{
					if (v.is_string())
					{
						outTarget.addInclude(k, v.get<std::string>());
					}
				}
			}
			else if (!isUnread(status))
				continue;

			if (String::equals("windows", key))
			{
				for (const auto& [k, v] : value.items())
				{
					if (v.is_boolean())
					{
						if (String::equals("includeRuntimeDlls", k))
							outTarget.setWindowsIncludeRuntimeDlls(v.get<bool>());
					}
				}
			}
			else if (String::equals("linuxDesktopEntry", key))
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
					else if (v.is_boolean())
					{
						if (String::equals("copyToApplications", k))
							outTarget.setLinuxCopyToApplications(v.get<bool>());
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
						else if (String::equals("iconMethod", k))
							outTarget.setMacosBundleIconMethod(v.get<std::string>());
						else if (String::equals("infoPropertyList", k))
							outTarget.setMacosBundleInfoPropertyList(v.get<std::string>());
						else if (String::equals("entitlementsPropertyList", k))
							outTarget.setMacosBundleEntitlementsPropertyList(v.get<std::string>());
					}
					else if (v.is_object())
					{
						if (String::equals("infoPropertyList", k))
						{
							outTarget.setMacosBundleInfoPropertyListContent(v.dump());
						}
						else if (String::equals("entitlementsPropertyList", k))
						{
							outTarget.setMacosBundleEntitlementsPropertyListContent(v.dump());
						}
					}
					else if (v.is_boolean())
					{
						if (String::equals("copyToApplications", k))
							outTarget.setMacosCopyToApplications(v.get<bool>());
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
			const Json& targetsJson = inRoot[Keys::Targets];
			if (targetsJson.is_object())
			{
				const StringList validKinds{ "executable", "sharedLibrary", "staticLibrary" };
				StringList targetList;
				for (auto& [name, targetJson] : targetsJson.items())
				{
					if (targetJson.is_object() && targetJson.contains(Keys::Kind))
					{
						const Json& targetKind = targetJson[Keys::Kind];
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
			const Json& targetsJson = inRoot[Keys::Targets];
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
				outTarget.setIconSize(static_cast<u16>(value.get<i32>()));
			if (String::equals("textSize", key))
				outTarget.setTextSize(static_cast<u16>(value.get<i32>()));
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
				i32 width = 0;
				i32 height = 0;
				for (const auto& [k, v] : value.items())
				{
					if (v.is_number())
					{
						if (String::equals("width", k))
							width = v.get<i32>();
						else if (String::equals("height", k))
							height = v.get<i32>();
					}
				}
				if (width > 0 && height > 0)
				{
					outTarget.setSize(static_cast<u16>(width), static_cast<u16>(height));
				}
			}
			else if (String::equals("positions", key))
			{
				for (const auto& [name, posJson] : value.items())
				{
					if (posJson.is_object())
					{
						i32 posX = 0;
						i32 posY = 0;
						for (const auto& [k, v] : posJson.items())
						{
							if (v.is_number())
							{
								if (String::equals("x", k))
									posX = v.get<i32>();
								else if (String::equals("y", k))
									posY = v.get<i32>();
							}
						}

						outTarget.addPosition(name, static_cast<i16>(posX), static_cast<i16>(posY));
					}
				}
			}
		}
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "dependsOn", status))
				outTarget.setDependsOn(std::move(val));
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
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "dependsOn", status))
				outTarget.setDependsOn(std::move(val));
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
bool ChaletJsonParser::parseDistributionValidation(ValidationDistTarget& outTarget, const Json& inNode) const
{
	bool hasSchema = false;
	bool hasFiles = false;
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			std::string val;
			if (valueMatchesSearchKeyPattern(val, value, key, "schema", status))
			{
				outTarget.setSchema(std::move(val));
				hasSchema = true;
			}
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "files", status))
			{
				outTarget.addFile(std::move(val));
				hasFiles = true;
			}
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "outputDescription", status))
				outTarget.setOutputDescription(std::move(val));
			else if (isInvalid(status))
				return false;
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "files", status))
			{
				outTarget.addFiles(std::move(val));
				hasFiles = true;
			}
			else if (isInvalid(status))
				return false;
		}
	}

	return hasSchema && hasFiles;
}

/*****************************************************************************/
std::optional<bool> ChaletJsonParser::parseTargetCondition(IBuildTarget& outTarget, const Json& inNode) const
{
	if (std::string val; json::assign(val, inNode, kCondition))
	{
		auto res = conditionIsValid(outTarget.name(), val);
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
	if (std::string val; json::assign(val, inNode, kCondition))
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
std::optional<bool> ChaletJsonParser::conditionIsValid(const std::string& inTargetName, const std::string& inContent) const
{
	if (!m_adapter.matchConditionVariables(inContent, [this, &inContent, &inTargetName](const std::string& key, const std::string& value, bool negate) {
			auto res = checkConditionVariable(inTargetName, inContent, key, value, negate);
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
std::optional<bool> ChaletJsonParser::conditionIsValid(const std::string& inContent) const
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
ConditionResult ChaletJsonParser::checkConditionVariable(const std::string& inTargetName, const std::string& inString, const std::string& key, const std::string& value, bool negate) const
{
	if (key.empty())
	{
		if (String::equals("runTarget", value))
		{
			auto& lastTarget = m_state.inputs.lastTarget();
			auto& buildTargets = getBuildTargets();
			const bool routeWillRun = m_state.inputs.route().willRun();
			if (routeWillRun && String::equals(Values::All, lastTarget))
				return ConditionResult::Pass;

			if (!routeWillRun || buildTargets.empty())
				return ConditionResult::Fail;

			if (negate)
			{
				if (String::equals(buildTargets, inTargetName))
					return ConditionResult::Fail;
			}
			else
			{
				if (!String::equals(buildTargets, inTargetName))
					return ConditionResult::Fail;
			}

			return ConditionResult::Pass;
		}
	}
	else if (String::equals("options", key))
	{
		if (String::equals("runTarget", value))
		{
			auto& lastTarget = m_state.inputs.lastTarget();

			auto& buildTargets = getBuildTargets();
			const bool routeWillRun = m_state.inputs.route().willRun();
			if (routeWillRun && String::equals(Values::All, lastTarget))
				return ConditionResult::Pass;

			if (!routeWillRun || buildTargets.empty())
				return ConditionResult::Fail;

			if (negate)
			{
				if (String::equals(buildTargets, inTargetName))
					return ConditionResult::Fail;
			}
			else
			{
				if (!String::equals(buildTargets, inTargetName))
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
ConditionResult ChaletJsonParser::checkConditionVariable(const std::string& inString, const std::string& key, const std::string& value, bool negate) const
{
	// LOG("  ", key, value, negate);

	constexpr auto conditionHasFailed = [](const bool& neg, const bool& condition) {
		return (neg && condition) || (!neg && !condition);
	};

	if (key.empty())
	{
		// :value or :{value} syntax
		if (String::equals("debug", value))
		{
			if (conditionHasFailed(negate, m_state.configuration.debugSymbols()))
				return ConditionResult::Fail;

			// if (negate)
			// {
			// 	if (m_state.configuration.debugSymbols())
			// 		return ConditionResult::Fail;
			// }
			// else
			// {
			// 	if (!m_state.configuration.debugSymbols())
			// 		return ConditionResult::Fail;
			// }
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
		if (conditionHasFailed(negate, String::equals(value, arch)))
			return ConditionResult::Fail;
	}
	else if (String::equals("configuration", key))
	{
		if (String::equals("debugSymbols", value))
		{
			if (conditionHasFailed(negate, m_state.configuration.debugSymbols()))
				return ConditionResult::Fail;
		}
		else if (String::equals("enableProfiling", value))
		{
			if (conditionHasFailed(negate, m_state.configuration.enableProfiling()))
				return ConditionResult::Fail;
		}
		else if (String::equals("interproceduralOptimization", value))
		{
			if (conditionHasFailed(negate, m_state.configuration.interproceduralOptimization()))
				return ConditionResult::Fail;
		}
		else
		{
			Diagnostic::error("Invalid condition '{}:{}' found in: {}", key, value, inString);
			return ConditionResult::Invalid;
		}
	}
	else if (String::equals("sanitize", key))
	{
		if (String::equals("address", value))
		{
			if (conditionHasFailed(negate, m_state.configuration.sanitizeAddress()))
				return ConditionResult::Fail;
		}
		else if (String::equals("hwaddress", value))
		{
			if (conditionHasFailed(negate, m_state.configuration.sanitizeHardwareAddress()))
				return ConditionResult::Fail;
		}
		else if (String::equals("memory", value))
		{
			if (conditionHasFailed(negate, m_state.configuration.sanitizeMemory()))
				return ConditionResult::Fail;
		}
		else if (String::equals("thread", value))
		{
			if (conditionHasFailed(negate, m_state.configuration.sanitizeThread()))
				return ConditionResult::Fail;
		}
		else if (String::equals("leak", value))
		{
			if (conditionHasFailed(negate, m_state.configuration.sanitizeLeaks()))
				return ConditionResult::Fail;
		}
		else if (String::equals("undefined", value))
		{
			if (conditionHasFailed(negate, m_state.configuration.sanitizeUndefinedBehavior()))
				return ConditionResult::Fail;
		}
	}
	else if (String::equals("env", key))
	{
		auto res = Environment::get(value.c_str());
		if (conditionHasFailed(negate, res != nullptr))
			return ConditionResult::Fail;

		// if (negate)
		// {
		// 	if (res != nullptr)
		// 		return ConditionResult::Fail;
		// }
		// else
		// {
		// 	if (res == nullptr)
		// 		return ConditionResult::Fail;
		// }
	}
	else
	{
		Diagnostic::error("Invalid condition property '{}' found in: {}", key, inString);
		return ConditionResult::Invalid;
	}

	return ConditionResult::Pass;
}

/*****************************************************************************/
bool ChaletJsonParser::valueMatchesSearchKeyPattern(const std::string& inKey, const char* inSearch, JsonNodeReadStatus& inStatus) const
{
	if (!String::equals(inSearch, inKey))
	{
		if (!String::startsWith(fmt::format("{}[", inSearch), inKey))
			return false;

		inStatus = JsonNodeReadStatus::ValidKeyUnreadValue;

		if (!m_adapter.matchConditionVariables(inKey, [this, &inKey, &inStatus](const std::string& key, const std::string& value, bool negate) {
				auto res = checkConditionVariable(inKey, key, value, negate);
				if (res == ConditionResult::Invalid)
					inStatus = JsonNodeReadStatus::Invalid;

				return res == ConditionResult::Pass;
			}))
		{
			if (m_adapter.lastOp == ConditionOp::InvalidOr)
			{
				inStatus = JsonNodeReadStatus::Invalid;
				Diagnostic::error("Syntax for AND '+', OR '|' are mutually exclusive. Both found in: {}", inKey);
			}
			return false;
		}
	}

	/*if (String::startsWith(fmt::format("{}.", inSearch), inKey))
	{
		LOG(inKey);
	}*/

	inStatus = JsonNodeReadStatus::ValidKeyReadValue;
	return true;
}
}
