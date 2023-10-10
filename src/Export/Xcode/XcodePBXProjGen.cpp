/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Xcode/XcodePBXProjGen.hpp"

#include "Compile/CommandAdapter/CommandAdapterClang.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Libraries/Json.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
struct SourceTargetGroup
{
	std::string path;
	std::string suffix;
	std::string outputFile;
	std::string intDir;
	StringList children;
	StringList sources;
	StringList headers;
	StringList dependencies;
	SourceKind kind = SourceKind::None;
};
enum class PBXFileEncoding : uint
{
	Default = 0,
	UTF8 = 4,
	UTF16 = 10,
	UTF16_BE = 2415919360,
	UTF16_LE = 2483028224,
	Western = 30,
	Japanese = 2147483649,
	TraditionalChinese = 2147483650,
	Korean = 2147483651,
	Arabic = 2147483652,
	Hebrew = 2147483653,
	Greek = 2147483654,
	Cyrillic = 2147483655,
	SimplifiedChinese = 2147483673,
	CentralEuropean = 2147483677,
	Turkish = 2147483683,
	Icelandic = 2147483685,
};

// Corresponds to minimum Xcode version the project format supports
//
/*enum class XcodeCompatibilityVersion
{
	Xcode_2_4 = 42,
	Xcode_3_0 = 44,
	Xcode_3_1 = 45,
	Xcode_3_2 = 46, // <-- Target this for now
	Xcode_X_X = 50,
	Xcode_X_X = 51,
};*/
constexpr int kMinimumObjectVersion = 46;
constexpr int kBuildActionMask = 2147483647;

/*****************************************************************************/
XcodePBXProjGen::XcodePBXProjGen(std::vector<Unique<BuildState>>& inStates) :
	m_states(inStates),
	// This is an arbitrary namespace guid to use for hashing
	m_xcodeNamespaceGuid("3C17F435-21B3-4D0A-A482-A276EDE1F0A2")
{
	UNUSED(m_states);
}

/*****************************************************************************/
bool XcodePBXProjGen::saveToFile(const std::string& inFilename)
{
	if (m_states.empty())
		return false;

	const auto& firstState = *m_states.front();
	const auto& workspaceName = firstState.workspace.metadata().name();
	const auto& workingDirectory = firstState.inputs.workingDirectory();
	const auto& outputDirectory = firstState.inputs.outputDirectory();

	m_projectUUID = Uuid::v5(fmt::format("{}_PBXPROJ", workspaceName), m_xcodeNamespaceGuid);
	m_projectGuid = m_projectUUID.str();

	StringList buildDirs{ outputDirectory };
	StringList buildConfigurations;
	std::map<std::string, SourceTargetGroup> groups;
	std::map<std::string, std::vector<const SourceTarget*>> targetPtrs;

	{
		for (auto& state : m_states)
		{
			const auto& configName = state->configuration.name();
			buildDirs.emplace_back(String::getPathFilename(state->paths.buildOutputDir()));
			buildConfigurations.emplace_back(configName);

			if (targetPtrs.find(configName) == targetPtrs.end())
				targetPtrs.emplace(configName, std::vector<const SourceTarget*>{});

			for (auto& target : state->targets)
			{
				if (target->isSources())
				{
					const auto& sourceTarget = static_cast<const SourceTarget&>(*target);
					const auto& name = sourceTarget.name();
					state->paths.setBuildDirectoriesBasedOnProjectKind(sourceTarget);

					// const auto& buildSuffix = sourceTarget.buildSuffix();

					if (groups.find(name) == groups.end())
					{
						groups.emplace(name, SourceTargetGroup{});
						groups[name].path = workingDirectory;
						groups[name].suffix = name;
						groups[name].outputFile = sourceTarget.outputFile();
						groups[name].kind = sourceTarget.kind();
					}

					targetPtrs[configName].push_back(static_cast<const SourceTarget*>(target.get()));

					groups[name].intDir = state->paths.intermediateDir(sourceTarget);

					auto& pch = sourceTarget.precompiledHeader();

					auto& sharedLinks = sourceTarget.projectSharedLinks();
					for (auto& link : sharedLinks)
						List::addIfDoesNotExist(groups[name].dependencies, link);

					auto& staticLinks = sourceTarget.projectStaticLinks();
					for (auto& link : staticLinks)
						List::addIfDoesNotExist(groups[name].dependencies, link);

					const auto& files = sourceTarget.files();
					for (auto& file : files)
					{
						List::addIfDoesNotExist(groups[name].sources, file);
						List::addIfDoesNotExist(groups[name].children, file);
					}

					if (!pch.empty())
					{
						List::addIfDoesNotExist(groups[name].headers, pch);
						List::addIfDoesNotExist(groups[name].children, pch);
					}
					auto tmpHeaders = sourceTarget.getHeaderFiles();
					for (auto& file : tmpHeaders)
					{
						List::addIfDoesNotExist(groups[name].headers, file);
						List::addIfDoesNotExist(groups[name].children, file);
					}
				}
			}
		}

		for (auto& [name, group] : groups)
		{
			std::sort(group.children.begin(), group.children.end());
			std::sort(group.sources.begin(), group.sources.end());
			std::sort(group.headers.begin(), group.headers.end());
		}
	}

	Json json;
	json["archiveVersion"] = 1;
	json["classes"] = Json::array();
	json["objectVersion"] = kMinimumObjectVersion;
	json["objects"] = Json::object();
	auto& objects = json.at("objects");

	auto mainGroup = Uuid::v5("mainGroup", m_xcodeNamespaceGuid).toAppleHash();
	auto products = getHashWithLabel("Products");
	const std::string group{ "SOURCE_ROOT" };

	// PBXBuildFile
	{
		const std::string section{ "PBXBuildFile" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (const auto& [target, pbxGroup] : groups)
		{
			if (!pbxGroup.intDir.empty())
			{
				auto key = getHashWithLabel(fmt::format("{} in Resources", pbxGroup.intDir));
				node[key]["isa"] = section;
				node[key]["fileRef"] = getHashedJsonValue(pbxGroup.intDir);
			}

			for (auto& file : pbxGroup.sources)
			{
				auto filename = getSourceWithSuffix(file, target);
				auto key = getHashWithLabel(fmt::format("{} in Sources", filename));
				node[key]["isa"] = section;
				node[key]["fileRef"] = getHashedJsonValue(getSourceWithSuffix(file, target));
			}
			for (auto& file : pbxGroup.headers)
			{
				auto filename = getSourceWithSuffix(file, target);
				auto key = getHashWithLabel(fmt::format("{} in Sources", filename));
				node[key]["isa"] = section;
				node[key]["fileRef"] = getHashedJsonValue(getSourceWithSuffix(file, target));
			}
		}
	}

	// PBXBuildStyle
	{
		/*
			3AF78447EE8141F2A29A3FF2 / MinSizeRel / = {
				isa = PBXBuildStyle;
				buildSettings = {
					COPY_PHASE_STRIP = NO;
				};
				name = MinSizeRel;
			};
		*/
		const std::string section{ "PBXBuildStyle" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (auto& configName : buildConfigurations)
		{
			auto key = getSectionKeyForTarget(configName, configName);
			node[key]["isa"] = section;
			node[key]["buildSettings"] = Json::object();
			node[key]["buildSettings"]["COPY_PHASE_STRIP"] = "NO";
			node[key]["name"] = configName;
		}
	}

	// PBXFileReference
	{
		const std::string section{ "PBXFileReference" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		// for (const auto& dir : intDirs)
		// {
		// 	auto key = getHashWithLabel(dir);
		// 	node[key]["isa"] = section;
		// 	node[key]["path"] = dir;
		// 	node[key]["sourceTree"] = group;
		// }
		struct ProjectFileSet
		{
			std::string file;
			std::string suffix;
			std::string fileType;
		};
		std::map<std::string, ProjectFileSet> projectFileList;
		for (const auto& [target, pbxGroup] : groups)
		{
			for (auto& file : pbxGroup.sources)
			{
				auto name = getSourceWithSuffix(file, target);
				auto type = firstState.paths.getSourceType(file);
				if (projectFileList.find(name) == projectFileList.end())
					projectFileList.emplace(name, ProjectFileSet{ file, target, getXcodeFileType(type) });
			}
		}
		for (const auto& [target, pbxGroup] : groups)
		{
			for (auto& file : pbxGroup.headers)
			{
				auto name = getSourceWithSuffix(file, target);
				auto ext = String::getPathSuffix(file);
				std::string type;
				if (String::equals('h', ext))
					type = "sourcecode.c.h";
				else
					type = "sourcecode.cpp.h";

				if (projectFileList.find(name) == projectFileList.end())
					projectFileList.emplace(name, ProjectFileSet{ file, target, type });
			}
		}

		for (auto& [name, set] : projectFileList)
		{
			auto key = getHashWithLabel(name);
			node[key]["isa"] = section;
			node[key]["explicitFileType"] = set.fileType;
			node[key]["fileEncoding"] = 4;
			node[key]["name"] = String::getPathFilename(set.file);
			// node[key]["path"] = fmt::format("{}/{}", workingDirectory, set.file);
			node[key]["path"] = set.file;
			node[key]["sourceTree"] = "SOURCE_ROOT";
		}
		for (const auto& [target, pbxGroup] : groups)
		{
			auto key = getHashWithLabel(target);
			node[key]["isa"] = section;
			node[key]["explicitFileType"] = getXcodeFileType(pbxGroup.kind);
			node[key]["includeInIndex"] = 0;
			node[key]["path"] = pbxGroup.outputFile;
			node[key]["sourceTree"] = "BUILT_PRODUCTS_DIR";
		}
	}

	// PBXContainerItemProxy
	{
		const std::string section{ "PBXContainerItemProxy" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (const auto& [target, pbxGroup] : groups)
		{
			auto hash = Uuid::v5(fmt::format("{}_TARGET", target), m_xcodeNamespaceGuid);

			auto key = getSectionKeyForTarget("PBXContainerItemProxy", target);
			node[key]["isa"] = section;
			node[key]["containerPortal"] = getHashWithLabel(m_projectUUID, "Project object");
			node[key]["proxyType"] = 1;
			node[key]["remoteGlobalIDString"] = hash.toAppleHash();
			node[key]["remoteInfo"] = target;
		}
	}

	// PBXGroup
	{
		const std::string section{ "PBXGroup" };
		objects[section] = Json::object();
		auto& node = objects.at(section);

		StringList childNodes;
		for (const auto& [target, pbxGroup] : groups)
		{
			auto key = getHashWithLabel(fmt::format("Sources [{}]", target));
			node[key]["isa"] = section;
			node[key]["children"] = Json::array();
			for (auto& child : pbxGroup.children)
			{
				node[key]["children"].push_back(getHashWithLabel(getSourceWithSuffix(child, target)));
			}
			node[key]["name"] = target;
			node[key]["path"] = pbxGroup.path;
			node[key]["sourceTree"] = group;
			childNodes.emplace_back(std::move(key));
		}

		node[products] = Json::object();
		node[products]["isa"] = section;
		node[products]["children"] = Json::array();
		for (const auto& [target, pbxGroup] : groups)
		{
			node[products]["children"].push_back(getHashWithLabel(target));
		}
		node[products]["name"] = "Products";
		node[products]["sourceTree"] = group;

		node[mainGroup] = Json::object();
		node[mainGroup]["isa"] = section;
		node[mainGroup]["children"] = childNodes;
		node[mainGroup]["children"].push_back(products);
		node[mainGroup]["sourceTree"] = group;
	}

	// PBXTargetDependency
	{
		const std::string section{ "PBXTargetDependency" };
		objects[section] = Json::object();
		auto& node = objects.at(section);

		for (auto& [target, _] : groups)
		{
			auto key = getSectionKeyForTarget("PBXTargetDependency", target);
			node[key]["isa"] = section;
			node[key]["target"] = getTargetHashWithLabel(target);
			node[key]["targetProxy"] = getSectionKeyForTarget("PBXContainerItemProxy", target);
		}
	}

	// PBXNativeTarget
	{
		const std::string section{ "PBXNativeTarget" };
		objects[section] = Json::object();
		auto& node = objects.at(section);

		for (const auto& [target, pbxGroup] : groups)
		{
			auto sources = getSectionKeyForTarget("Sources", target);
			auto resources = getSectionKeyForTarget("Resources", target);
			auto key = getTargetHashWithLabel(target);
			node[key]["isa"] = section;
			node[key]["buildConfigurationList"] = getHashWithLabel(getBuildConfigurationListLabel(target));
			node[key]["buildPhases"] = Json::array();
			node[key]["buildPhases"].push_back(sources);
			node[key]["buildPhases"].push_back(resources);
			node[key]["buildRules"] = Json::array();
			node[key]["dependencies"] = Json::array();
			for (auto& dependency : pbxGroup.dependencies)
			{
				node[key]["dependencies"].push_back(getSectionKeyForTarget("PBXTargetDependency", dependency));
			}
			node[key]["name"] = target;
			node[key]["productName"] = target;
			node[key]["productReference"] = getHashWithLabel(target);
			node[key]["productType"] = getNativeProductType(pbxGroup.kind);
		}
	}

	// PBXProject
	{
		const std::string section{ "PBXProject" };
		const std::string region{ "en" };
		const std::string name{ "project" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		auto key = getHashWithLabel(m_projectUUID, "Project object");
		node[key]["isa"] = section;
		node[key]["attributes"] = Json::object();
		node[key]["attributes"]["BuildIndependentTargetsInParallel"] = "YES";
		node[key]["attributes"]["LastUpgradeCheck"] = 1430;
		// node[key]["attributes"]["TargetAttributes"] = Json::object();
		node[key]["buildConfigurationList"] = getHashWithLabel(getBuildConfigurationListLabel(name, false));
		node[key]["buildSettings"] = Json::object();
		node[key]["buildStyles"] = Json::array();
		for (auto& configName : buildConfigurations)
		{
			node[key]["buildStyles"].push_back(getSectionKeyForTarget(configName, configName));
		}
		node[key]["compatibilityVersion"] = "Xcode 11.0";
		node[key]["developmentRegion"] = region;
		node[key]["hasScannedForEncodings"] = 0;
		node[key]["knownRegions"] = Json::array();
		node[key]["knownRegions"].push_back("Base");
		node[key]["knownRegions"].push_back(region);
		node[key]["mainGroup"] = mainGroup;
		node[key]["projectDirPath"] = workingDirectory;
		node[key]["projectRoot"] = "";
		node[key]["targets"] = Json::array();
		for (const auto& [target, _] : groups)
		{
			node[key]["targets"].push_back(getTargetHashWithLabel(target));
		}
	}

	// PBXResourcesBuildPhase
	{
		const std::string section{ "PBXResourcesBuildPhase" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (const auto& [target, pbxGroup] : groups)
		{
			if (pbxGroup.intDir.empty())
				continue;

			auto key = getSectionKeyForTarget("Resources", target);
			node[key]["isa"] = section;
			node[key]["buildActionMask"] = kBuildActionMask;
			node[key]["files"] = Json::array();

			node[key]["isa"] = section;
			node[key]["buildActionMask"] = kBuildActionMask;
			node[key]["files"] = Json::array();

			auto filename = getSourceWithSuffix(pbxGroup.intDir, target);
			node[key]["files"].push_back(getHashWithLabel(fmt::format("{} in Resources", pbxGroup.intDir)));

			node[key]["runOnlyForDeploymentPostprocessing"] = 0;
		}
	}

	// PBXSourcesBuildPhase
	{
		const std::string section{ "PBXSourcesBuildPhase" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (const auto& [target, pbxGroup] : groups)
		{
			auto key = getSectionKeyForTarget("Sources", target);
			node[key]["isa"] = section;
			node[key]["buildActionMask"] = kBuildActionMask;
			node[key]["files"] = Json::array();

			for (const auto& file : pbxGroup.sources)
			{
				auto filename = getSourceWithSuffix(file, target);
				node[key]["files"].push_back(getHashWithLabel(fmt::format("{} in Sources", filename)));
			}

			node[key]["runOnlyForDeploymentPostprocessing"] = 0;
		}
	}

	// LOG(json.dump(2, ' '));

	// XCBuildConfiguration
	{
		std::map<std::string, BuildState*> statePtrs;
		for (auto& state : m_states)
		{
			const auto& configName = state->configuration.name();
			if (statePtrs.find(configName) == statePtrs.end())
				statePtrs.emplace(configName, state.get());
		}

		const std::string section{ "XCBuildConfiguration" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (auto& state : m_states)
		{
			auto& name = state->configuration.name();
			auto hash = Uuid::v5(fmt::format("{}_PROJECT", name), m_xcodeNamespaceGuid);
			auto key = getHashWithLabel(hash, name);
			node[key]["isa"] = section;
			node[key]["buildSettings"] = getProductBuildSettings(*state);
			// node[key]["baseConfigurationReference"] = "";
			node[key]["name"] = name;
		}
		for (auto& [name, state] : statePtrs)
		{
			auto& configTargets = targetPtrs.at(name);
			for (auto& target : configTargets)
			{
				auto hash = Uuid::v5(fmt::format("{}-{}", name, target->name()), m_xcodeNamespaceGuid);
				auto key = getHashWithLabel(hash, name);
				node[key]["isa"] = section;
				node[key]["buildSettings"] = getBuildSettings(*state, *target);
				// node[key]["baseConfigurationReference"] = "";
				node[key]["name"] = name;
			}
		}
	}

	// XCConfigurationList
	{
		const std::string project{ "project" };
		const std::string section{ "XCConfigurationList" };
		objects[section] = Json::object();

		StringList configurations;
		for (auto& configName : buildConfigurations)
		{
			auto hash = Uuid::v5(fmt::format("{}_PROJECT", configName), m_xcodeNamespaceGuid);
			configurations.emplace_back(getHashWithLabel(hash, configName));
		}

		auto& node = objects.at(section);
		{
			auto key = getHashWithLabel(getBuildConfigurationListLabel(project, false));
			node[key]["isa"] = section;
			node[key]["buildConfigurations"] = configurations;
			node[key]["defaultConfigurationIsVisible"] = 0;
			node[key]["defaultConfigurationName"] = firstState.configuration.name();
		}

		for (const auto& [target, _] : groups)
		{
			configurations.clear();
			for (auto& name : buildConfigurations)
			{
				auto hash = Uuid::v5(fmt::format("{}-{}", name, target), m_xcodeNamespaceGuid);
				configurations.emplace_back(getHashWithLabel(hash, name));
			}

			auto key = getHashWithLabel(getBuildConfigurationListLabel(target));
			node[key]["isa"] = section;
			node[key]["buildConfigurations"] = configurations;
			node[key]["defaultConfigurationIsVisible"] = 0;
			node[key]["defaultConfigurationName"] = firstState.configuration.name();
		}
	}

	json["rootObject"] = getHashedJsonValue(m_projectUUID, "Project object");

	// LOG(json.dump(2, ' '));

	auto contents = generateFromJson(json);
	bool replaceContents = true;

	if (Commands::pathExists(inFilename))
	{
		auto contentHash = Hash::uint64(contents);
		auto existing = Commands::getFileContents(inFilename);
		if (!existing.empty())
		{
			existing.pop_back();
			auto existingHash = Hash::uint64(existing);
			replaceContents = existingHash != contentHash;
		}
	}

	// LOG(contents);

	if (replaceContents && !Commands::createFileWithContents(inFilename, contents))
	{
		Diagnostic::error("There was a problem creating the Xcode project: {}", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
std::string XcodePBXProjGen::getHashWithLabel(const std::string& inValue) const
{
	auto hash = Uuid::v5(inValue, m_xcodeNamespaceGuid);
	return getHashWithLabel(hash, inValue);
}

/*****************************************************************************/
std::string XcodePBXProjGen::getHashWithLabel(const Uuid& inHash, const std::string& inLabel) const
{
	return fmt::format("{} /* {} */", inHash.toAppleHash(), inLabel);
}

/*****************************************************************************/
std::string XcodePBXProjGen::getTargetHashWithLabel(const std::string& inTarget) const
{
	return getHashWithLabel(Uuid::v5(fmt::format("{}_TARGET", inTarget), m_xcodeNamespaceGuid), inTarget);
}

/*****************************************************************************/
std::string XcodePBXProjGen::getSectionKeyForTarget(const std::string& inKey, const std::string& inTarget) const
{
	return getHashWithLabel(Uuid::v5(fmt::format("{}_KEY [{}]", inKey, inTarget), m_xcodeNamespaceGuid), inKey);
}

/*****************************************************************************/
Json XcodePBXProjGen::getHashedJsonValue(const std::string& inValue) const
{
	auto hash = Uuid::v5(inValue, m_xcodeNamespaceGuid);
	return getHashedJsonValue(hash, inValue);
}

/*****************************************************************************/
Json XcodePBXProjGen::getHashedJsonValue(const Uuid& inHash, const std::string& inLabel) const
{
	// Json ret = Json::object();
	// ret["hash"] = inHash.toAppleHash();
	// ret["label"] = inLabel;
	// return ret;
	return Json(getHashWithLabel(inHash, inLabel));
}

/*****************************************************************************/
std::string XcodePBXProjGen::getBuildConfigurationListLabel(const std::string& inName, const bool inNativeProject) const
{
	auto type = inNativeProject ? "PBXNativeTarget" : "PBXProject";
	return fmt::format("Build configuration list for {} \"{}\"", type, inName);
}

/*****************************************************************************/
Json XcodePBXProjGen::getBuildSettings(BuildState& inState, const SourceTarget& inTarget) const
{
	CommandAdapterClang clangAdapter(inState, inTarget);

	const auto& cwd = inState.inputs.workingDirectory();
	auto lang = inTarget.language();
	inState.paths.setBuildDirectoriesBasedOnProjectKind(inTarget);

	// TODO: this is currently just based on a Release mode

	Json ret;

	// ret["ALWAYS_SEARCH_USER_PATHS"] = getBoolString(false);
	ret["BUILD_DIR"] = fmt::format("{}/{}", cwd, inState.paths.outputDirectory());
	ret["CLANG_ANALYZER_NONNULL"] = getBoolString(true);
	ret["CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION"] = "YES_AGGRESSIVE";

	auto cStandard = clangAdapter.getLanguageStandardC();
	auto cppStandard = clangAdapter.getLanguageStandardCpp();

	if (!cppStandard.empty())
		ret["CLANG_CXX_LANGUAGE_STANDARD"] = std::move(cppStandard);

	ret["CLANG_CXX_LIBRARY"] = clangAdapter.getCxxLibrary();

	if (inTarget.objectiveCxx())
	{
		ret["CLANG_ENABLE_MODULES"] = getBoolString(true);
		ret["CLANG_ENABLE_OBJC_ARC"] = getBoolString(true);
		ret["CLANG_ENABLE_OBJC_WEAK"] = getBoolString(true);
	}

	ret["CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING"] = getBoolString(true);
	ret["CLANG_WARN_BOOL_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_COMMA"] = getBoolString(true);
	ret["CLANG_WARN_CONSTANT_CONVERSION"] = getBoolString(true);

	if (inTarget.objectiveCxx())
	{
		ret["CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS"] = getBoolString(true);
		ret["CLANG_WARN_DIRECT_OBJC_ISA_USAGE"] = "YES_ERROR";
	}

	ret["CLANG_WARN_DOCUMENTATION_COMMENTS"] = getBoolString(true);
	ret["CLANG_WARN_EMPTY_BODY"] = getBoolString(true);
	ret["CLANG_WARN_ENUM_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_INFINITE_RECURSION"] = getBoolString(true);
	ret["CLANG_WARN_INT_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_NON_LITERAL_NULL_CONVERSION"] = getBoolString(true);

	if (inTarget.objectiveCxx())
	{
		ret["CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF"] = getBoolString(true);
		ret["CLANG_WARN_OBJC_LITERAL_CONVERSION"] = getBoolString(true);
		ret["CLANG_WARN_OBJC_ROOT_CLASS"] = "YES_ERROR";
	}

	ret["CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER"] = getBoolString(true);
	ret["CLANG_WARN_RANGE_LOOP_ANALYSIS"] = getBoolString(true);
	ret["CLANG_WARN_STRICT_PROTOTYPES"] = getBoolString(true);
	ret["CLANG_WARN_SUSPICIOUS_MOVE"] = getBoolString(true);
	ret["CLANG_WARN_UNGUARDED_AVAILABILITY"] = "YES_AGGRESSIVE";
	ret["CLANG_WARN_UNREACHABLE_CODE"] = getBoolString(true);
	ret["CLANG_WARN__DUPLICATE_METHOD_MATCH"] = getBoolString(true);
	// ret["COMBINE_HIDPI_IMAGES"] = getBoolString(true);

	// ret["CODE_SIGN_IDENTITY"] = inState.tools.signingIdentity();
	ret["CODE_SIGN_IDENTITY"] = "-";
	ret["CODE_SIGN_INJECT_BASE_ENTITLEMENTS"] = getBoolString(true);

	ret["CONFIGURATION_BUILD_DIR"] = fmt::format("{}/{}", cwd, inState.paths.buildOutputDir());

	// Note: can't use buildSuffix
	auto newObjDir = fmt::format("{}/{}/obj.{}", cwd, inState.paths.buildOutputDir(), inTarget.name());
	ret["CONFIGURATION_TEMP_DIR"] = newObjDir;
	// ret["COPY_PHASE_STRIP"] = getBoolString(false);
	// ret["ENABLE_NS_ASSERTIONS"] = getBoolString(false);

	// include dirs
	{
		StringList includes;
		const auto& includeDirs = inTarget.includeDirs();
		const auto& objDir = inState.paths.objDir();
		const auto& intDir = inState.paths.intermediateDir(inTarget);
		for (auto& include : includeDirs)
		{
			if (String::equals(objDir, include))
			{
				includes.emplace_back(newObjDir);
			}
			else
			{
				auto temp = fmt::format("{}/{}", cwd, include);
				if (String::equals(intDir, include) || Commands::pathExists(temp))
					includes.emplace_back(std::move(temp));
				else
					includes.emplace_back(include);
			}
		}
		ret["HEADER_SEARCH_PATHS"] = std::move(includes);
	}

	if (inTarget.objectiveCxx())
	{
		ret["ENABLE_STRICT_OBJC_MSGSEND"] = getBoolString(true);
	}

	if (inTarget.isStaticLibrary())
	{
		ret["EXECUTABLE_PREFIX"] = "lib";
		ret["EXECUTABLE_SUFFIX"] = ".a";
	}
	else if (inTarget.isSharedLibrary())
	{
		ret["EXECUTABLE_PREFIX"] = "lib";
		ret["EXECUTABLE_SUFFIX"] = ".dylib";
	}

	ret["FRAMEWORK_FLAG_PREFIX"] = "-framework";
	ret["LIBRARY_FLAG_PREFIX"] = "-l";

	if (inTarget.usesPrecompiledHeader())
	{
		ret["GCC_PREFIX_HEADER"] = fmt::format("{}/{}", cwd, inTarget.precompiledHeader());
		ret["GCC_PRECOMPILE_PREFIX_HEADER"] = getBoolString(true);
	}

	if (!cStandard.empty())
		ret["GCC_C_LANGUAGE_STANDARD"] = std::move(cStandard);

	ret["GCC_NO_COMMON_BLOCKS"] = getBoolString(true);
	ret["GCC_WARN_64_TO_32_BIT_CONVERSION"] = getBoolString(true);
	ret["GCC_WARN_ABOUT_RETURN_TYPE"] = "YES_ERROR";
	ret["GCC_WARN_UNDECLARED_SELECTOR"] = getBoolString(true);
	ret["GCC_WARN_UNINITIALIZED_AUTOS"] = "YES_AGGRESSIVE";
	ret["GCC_WARN_UNUSED_FUNCTION"] = getBoolString(true);
	ret["GCC_WARN_UNUSED_VARIABLE"] = getBoolString(true);
	ret["LD_RUNPATH_SEARCH_PATHS"] = {
		"$(inherited)",
		"@executable_path/../Frameworks",
	};
	ret["MACH_O_TYPE"] = getMachOType(inTarget);
	ret["MACOSX_DEPLOYMENT_TARGET"] = inState.inputs.osTargetVersion();
	// ret["MTL_ENABLE_DEBUG_INFO"] = getBoolString(inState.configuration.debugSymbols());
	// ret["MTL_FAST_MATH"] = getBoolString(false);
	ret["OBJECT_FILE_DIR"] = ret.at("CONFIGURATION_TEMP_DIR");

	const auto& compileOptions = inTarget.compileOptions();
	if (!compileOptions.empty())
	{
		if (lang == CodeLanguage::C || lang == CodeLanguage::ObjectiveC)
		{
			ret["OTHER_CFLAGS"] = compileOptions;
		}
		else if (lang == CodeLanguage::CPlusPlus || lang == CodeLanguage::ObjectiveCPlusPlus)
		{
			ret["OTHER_CPLUSPLUSFLAGS"] = compileOptions;
		}
	}

	{

		const auto& linkerOptions = inTarget.linkerOptions();
		if (!linkerOptions.empty())
		{
			ret["OTHER_LDFLAGS"] = linkerOptions;
		}
		ret["OTHER_LDFLAGS"] = Json::array();
		const auto& links = inTarget.links();
		for (auto& link : links)
		{
			ret["OTHER_LDFLAGS"].push_back(fmt::format("-l{}", link));
		}
		const auto& staticLinks = inTarget.staticLinks();
		for (auto& link : staticLinks)
		{
			ret["OTHER_LDFLAGS"].push_back(fmt::format("-l{}", link));
		}
	}

	ret["PRODUCT_BUNDLE_IDENTIFIER"] = getProductBundleIdentifier(inState.workspace.metadata().name());
	ret["SDKROOT"] = inState.inputs.osTargetName();
	ret["TARGET_TEMP_DIR"] = ret.at("CONFIGURATION_TEMP_DIR");

	// ret["BUILD_ROOT"] = fmt::format("{}/{}", cwd, inState.paths.buildOutputDir());
	// ret["SWIFT_OBJC_BRIDGING_HEADER"] = "";

	return ret;
}

/*****************************************************************************/
Json XcodePBXProjGen::getProductBuildSettings(const BuildState& inState) const
{
	const auto& config = inState.configuration;
	SourceTarget dummyTarget(inState);
	CommandAdapterClang clangAdapter(inState, dummyTarget);

	Json ret;
	ret["ALWAYS_SEARCH_USER_PATHS"] = getBoolString(false);
	ret["ARCHES"] = inState.info.targetArchitectureString();
	ret["COPY_PHASE_STRIP"] = getBoolString(false);
	if (config.debugSymbols())
	{
		ret["DEBUG_INFORMATION_FORMAT"] = "dwarf-with-dsym";
	}
	ret["ENABLE_TESTABILITY"] = getBoolString(true);
	ret["GENERATE_PROFILING_CODE"] = getBoolString(config.enableProfiling());
	ret["GCC_GENERATE_DEBUGGING_SYMBOLS"] = getBoolString(config.debugSymbols());
	ret["GCC_OPTIMIZATION_LEVEL"] = clangAdapter.getOptimizationLevel();
	// ret["GCC_PREPROCESSOR_DEFINITIONS"] = {
	// 	"$(inherited)",
	// 	"DEBUG=1",
	// };

	// YES, YES_THIN, NO
	//   TODO: thin = incremental - maybe add in the future?
	ret["LLVM_LTO"] = getBoolString(config.interproceduralOptimization());

	ret["ONLY_ACTIVE_ARCH"] = getBoolString(true);
	ret["PRODUCT_NAME"] = "$(TARGET_NAME)";
	ret["SDKROOT"] = inState.inputs.osTargetName();
	// ret["LD_RUNPATH_SEARCH_PATHS"] = {
	// 	"$(inherited)",
	// 	"@executable_path/../Frameworks",
	// 	"@loader_path/../Frameworks",
	// };

	// Code signing
	// ret["CODE_SIGN_STYLE"] = "Automatic";
	// ret["DEVELOPMENT_TEAM"] = ""; // required!

	return ret;
}

/*****************************************************************************/
std::string XcodePBXProjGen::getBoolString(const bool inValue) const
{
	return inValue ? "YES" : "NO";
}

/*****************************************************************************/
std::string XcodePBXProjGen::getProductBundleIdentifier(const std::string& inWorkspaceName) const
{
	// TODO - appleProductBundleIdentiifer or something
	return fmt::format("com.myapp.{}", inWorkspaceName);
}

/*****************************************************************************/
std::string XcodePBXProjGen::getXcodeFileType(const SourceType inType) const
{
	switch (inType)
	{
		case SourceType::ObjectiveCPlusPlus:
			return "sourcecode.cpp.objcpp";
		case SourceType::ObjectiveC:
			return "sourcecode.c.objc";
		case SourceType::C:
			return "sourcecode.c.c";
		case SourceType::CPlusPlus:
		default:
			return "sourcecode.cpp.cpp";
	}
}

/*****************************************************************************/
std::string XcodePBXProjGen::getXcodeFileType(const SourceKind inKind) const
{
	switch (inKind)
	{
		case SourceKind::Executable:
			return "compiled.mach-o.executable";
		case SourceKind::SharedLibrary:
			return "compiled.mach-o.dylib";
		case SourceKind::StaticLibrary:
			return "archive.ar";
		default:
			return std::string();
	}
}

/*****************************************************************************/
std::string XcodePBXProjGen::getMachOType(const SourceTarget& inTarget) const
{
	if (inTarget.isStaticLibrary())
		return "staticlib";
	else if (inTarget.isSharedLibrary())
		return "mh_dylib";
	else
		return "mh_execute";
}

/*****************************************************************************/
std::string XcodePBXProjGen::getNativeProductType(const SourceKind inKind) const
{
	/*
		com.apple.product-type.library.static
		com.apple.product-type.library.dynamic
		com.apple.product-type.tool
		com.apple.product-type.application
	*/
	switch (inKind)
	{
		case SourceKind::Executable:
			return "com.apple.product-type.tool";
		case SourceKind::SharedLibrary:
			return "com.apple.product-type.library.dynamic";
		case SourceKind::StaticLibrary:
			return "com.apple.product-type.library.static";
		default:
			return std::string();
	}
}

/*****************************************************************************/
std::string XcodePBXProjGen::generateFromJson(const Json& inJson) const
{

	int archiveVersion = inJson["archiveVersion"].get<int>();
	int objectVersion = inJson["objectVersion"].get<int>();
	auto rootObject = inJson["rootObject"].get<std::string>();

	StringList singleLineSections{
		"PBXBuildFile",
		"PBXFileReference",
	};

	std::string sections;
	const auto& objects = inJson.at("objects");
	for (auto& [key, value] : objects.items())
	{
		const auto& section = key;
		if (!value.is_object())
			continue;

		int indent = 2;
		if (String::equals(singleLineSections, section))
			indent = 0;

		sections += fmt::format("\n/* Begin {} section */\n", section);
		for (auto& [subkey, subvalue] : value.items())
		{
			if (!subvalue.is_object())
				continue;

			sections += std::string(2, '\t');
			sections += subkey;
			sections += " = ";
			sections += getNodeAsPListFormat(subvalue, indent);
			sections += ";\n";
		}
		sections += fmt::format("/* End {} section */\n", section);
	}

	sections.pop_back();

	auto contents = fmt::format(R"pbxproj(// !$*UTF8*$!
{{
	archiveVersion = {archiveVersion};
	classes = {{
	}};
	objectVersion = {objectVersion};
	objects = {{
{sections}
	}};
	rootObject = {rootObject};
}})pbxproj",
		FMT_ARG(archiveVersion),
		FMT_ARG(objectVersion),
		FMT_ARG(sections),
		FMT_ARG(rootObject));

	return contents;
}

/*****************************************************************************/
std::string XcodePBXProjGen::getNodeAsPListFormat(const Json& inJson, const size_t indent) const
{
	std::string ret;

	if (inJson.is_object())
	{
		ret += "{\n";

		if (inJson.contains("isa"))
		{
			const auto value = inJson.at("isa").get<std::string>();
			ret += std::string(indent + 1, '\t');
			ret += fmt::format("isa = {};\n", value);
		}

		for (auto& [key, value] : inJson.items())
		{
			if (String::equals("isa", key))
				continue;

			ret += std::string(indent + 1, '\t');
			ret += key;
			ret += " = ";
			ret += getNodeAsPListFormat(value, indent + 1);
			ret += ";\n";
		}
		ret += std::string(indent, '\t');
		ret += '}';
	}
	else if (inJson.is_array())
	{
		ret += "(\n";
		for (auto& value : inJson)
		{
			ret += std::string(indent + 1, '\t');
			ret += getNodeAsPListString(value);
			ret += ",\n";
		}

		// removes last comma
		// ret.pop_back();
		// ret.pop_back();
		// ret += '\n';

		ret += std::string(indent, '\t');
		ret += ')';
	}
	else if (inJson.is_string())
	{
		ret += getNodeAsPListString(inJson);
	}
	else if (inJson.is_number_float())
	{
		ret += std::to_string(inJson.get<float>());
	}
	else if (inJson.is_number_integer())
	{
		ret += std::to_string(inJson.get<std::int64_t>());
	}
	else if (inJson.is_number_unsigned())
	{
		ret += std::to_string(inJson.get<std::uint64_t>());
	}

	if (indent == 0)
	{
		String::replaceAll(ret, '\n', ' ');
		String::replaceAll(ret, '\t', "");
	}

	return ret;
}

/*****************************************************************************/
std::string XcodePBXProjGen::getNodeAsPListString(const Json& inJson) const
{
	if (!inJson.is_string())
		return "\"\"";

	bool startsWithHash = false;
	auto str = inJson.get<std::string>();
	if (str.size() > 24)
	{
		auto substring = str.substr(0, 23);
		if (substring.find_first_not_of("01234567890ABCDEF") == std::string::npos)
		{
			startsWithHash = true;
		}
	}

	if (!str.empty() && (startsWithHash || str.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_/") == std::string::npos))
	{
		return str;
	}
	else
	{
		return fmt::format("\"{}\"", str);
	}
}

/*****************************************************************************/
std::string XcodePBXProjGen::getSourceWithSuffix(const std::string& inFile, const std::string& inSuffix) const
{
	return fmt::format("[{}] {}", inSuffix, inFile);
}
}
