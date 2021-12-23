/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "ChaletJson/CentralChaletJsonParser.hpp"

#include "ChaletJson/ChaletJsonSchema.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/CentralState.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonKeys.hpp"

#include "State/Distribution/BundleArchiveTarget.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/MacosDiskImageTarget.hpp"
#include "State/Distribution/ProcessDistTarget.hpp"
#include "State/Distribution/ScriptDistTarget.hpp"
#include "State/Distribution/WindowsNullsoftInstallerTarget.hpp"

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

	if (!parseDefaultConfigurations(inNode))
		return false;

	if (!parseConfigurations(inNode))
		return false;

	if (!parseExternalDependencies(inNode))
		return false;

	if (m_centralState.inputs().route() != Route::Configure)
	{
		if (!parseDistribution(inNode))
			return false;
	}

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

	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			if (String::equals("workspace", key))
				m_centralState.workspace.setWorkspaceName(value.get<std::string>());
			else if (String::equals("version", key))
				m_centralState.workspace.setVersion(value.get<std::string>());
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "searchPaths", status))
				m_centralState.workspace.addSearchPaths(std::move(val));
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

					if (m_centralState.releaseConfiguration().empty())
					{
						if (!config.isDebuggable())
						{
							m_centralState.setReleaseConfiguration(config.name());
						}
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
					JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
					if (value.is_string())
					{
						if (String::equals("optimizationLevel", key))
							config.setOptimizationLevel(value.get<std::string>());
					}
					else if (value.is_boolean())
					{
						if (String::equals("linkTimeOptimization", key))
							config.setLinkTimeOptimization(value.get<bool>());
						if (String::equals("stripSymbols", key))
							config.setStripSymbols(value.get<bool>());
						if (String::equals("debugSymbols", key))
							config.setDebugSymbols(value.get<bool>());
						if (String::equals("enableProfiling", key))
							config.setEnableProfiling(value.get<bool>());
					}
					else if (value.is_array())
					{
						StringList val;
						if (valueMatchesSearchKeyPattern(val, value, key, "sanitize", status))
							config.addSanitizeOptions(std::move(val));
					}
				}

				if (m_centralState.releaseConfiguration().empty())
				{
					if (!config.isDebuggable())
					{
						m_centralState.setReleaseConfiguration(config.name());
					}
				}

				m_centralState.addBuildConfiguration(name, std::move(config));
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool CentralChaletJsonParser::parseDistribution(const Json& inNode) const
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
			else if (String::equals("windowsNullsoftInstaller", val))
			{
#if defined(CHALET_WIN32) || defined(CHALET_LINUX)
				type = DistTargetType::WindowsNullsoftInstaller;
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

		DistTarget target = IDistTarget::make(type);
		target->setName(name);

		if (target->isScript())
		{
			if (!parseDistributionScript(static_cast<ScriptDistTarget&>(*target), targetJson))
				continue;
		}
		else if (target->isProcess())
		{
			if (!parseDistributionProcess(static_cast<ProcessDistTarget&>(*target), targetJson))
				continue;
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
		else if (target->isWindowsNullsoftInstaller())
		{
			if (!parseWindowsNullsoftInstaller(static_cast<WindowsNullsoftInstallerTarget&>(*target), targetJson))
				return false;
		}

		if (!target->includeInDistribution())
			continue;

		m_centralState.distribution.emplace_back(std::move(target));
	}

	return true;
}

/*****************************************************************************/
bool CentralChaletJsonParser::parseDistributionScript(ScriptDistTarget& outTarget, const Json& inNode) const
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

	return valid;
}

/*****************************************************************************/
bool CentralChaletJsonParser::parseDistributionProcess(ProcessDistTarget& outTarget, const Json& inNode) const
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
				outTarget.addArguments(std::move(val));
		}
	}

	return valid;
}

/*****************************************************************************/
bool CentralChaletJsonParser::parseDistributionArchive(BundleArchiveTarget& outTarget, const Json& inNode) const
{
	if (!parseTargetCondition(outTarget, inNode))
		return true;

	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			std::string val;
			if (valueMatchesSearchKeyPattern(val, value, key, "description", status))
				outTarget.setDescription(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "include", status))
				outTarget.addInclude(std::move(val));
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "include", status))
				outTarget.addIncludes(std::move(val));
		}
	}

	return true;
}

/*****************************************************************************/
bool CentralChaletJsonParser::parseDistributionBundle(BundleTarget& outTarget, const Json& inNode, const Json& inRoot) const
{
	if (!parseTargetCondition(outTarget, inNode))
		return true;

	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			if (String::equals("configuration", key))
			{
				outTarget.setConfiguration(value.get<std::string>());
				m_centralState.addRequiredBuildConfiguration(outTarget.configuration());
			}
			else if (String::equals("buildTargets", key))
				outTarget.addBuildTarget(value.get<std::string>());
			else if (String::equals("description", key))
				outTarget.setDescription(value.get<std::string>());
			else if (String::equals("subdirectory", key))
				outTarget.setSubdirectory(value.get<std::string>());
			else if (String::equals("mainExecutable", key))
				outTarget.setMainExecutable(value.get<std::string>());
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "buildTargets", status))
				outTarget.addBuildTargets(std::move(val));
			if (valueMatchesSearchKeyPattern(val, value, key, "include", status))
				outTarget.addIncludes(std::move(val));
			if (valueMatchesSearchKeyPattern(val, value, key, "exclude", status))
				outTarget.addExcludes(std::move(val));
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
bool CentralChaletJsonParser::parseMacosDiskImage(MacosDiskImageTarget& outTarget, const Json& inNode) const
{
	for (const auto& [key, value] : inNode.items())
	{
		if (value.is_string())
		{
			if (String::equals("description", key))
				outTarget.setDescription(value.get<std::string>());
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
bool CentralChaletJsonParser::parseWindowsNullsoftInstaller(WindowsNullsoftInstallerTarget& outTarget, const Json& inNode) const
{
	for (const auto& [key, value] : inNode.items())
	{
		JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
		if (value.is_string())
		{
			std::string val;
			if (String::equals("description", key))
				outTarget.setDescription(value.get<std::string>());
			else if (String::equals("file", key))
				outTarget.setFile(value.get<std::string>());
		}
		else if (value.is_array())
		{
			StringList val;
			if (valueMatchesSearchKeyPattern(val, value, key, "pluginDirs", status))
				outTarget.addPluginDirs(std::move(val));
			else if (isUnread(status) && valueMatchesSearchKeyPattern(val, value, key, "defines", status))
				outTarget.addDefines(std::move(val));
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

/*****************************************************************************/
bool CentralChaletJsonParser::parseTargetCondition(IDistTarget& outTarget, const Json& inNode) const
{
	if (std::string val; m_chaletJson.assignFromKey(val, inNode, "condition"))
	{
		outTarget.setIncludeInDistribution(conditionIsValid(val));
	}

	return outTarget.includeInDistribution();
}

/*****************************************************************************/
/*****************************************************************************/
bool CentralChaletJsonParser::conditionIsValid(const std::string& inContent) const
{
	if (!String::equals(m_platform, inContent))
	{
		auto split = String::split(inContent, '.');

		if (List::contains(split, fmt::format("!{}", m_platform)))
			return false;

		for (auto& notPlatform : m_notPlatforms)
		{
			if (List::contains(split, notPlatform))
				return false;
		}

		const auto incCi = Environment::isContinuousIntegrationServer() ? "" : "!";
		if (!List::contains(split, fmt::format("{}ci", incCi)))
			return false;
	}

	return true;
}
}
