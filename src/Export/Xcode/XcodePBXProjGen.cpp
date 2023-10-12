/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Xcode/XcodePBXProjGen.hpp"

#include "Compile/CommandAdapter/CommandAdapterClang.hpp"
#include "Compile/CompilerCxx/CompilerCxxGCC.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/Xcode/OldPListGenerator.hpp"
#include "Export/Xcode/TargetAdapterPBXProj.hpp"
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
enum class PBXGroupKind : ushort
{
	Source,
	Script,
	BuildAll,
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
struct SourceTargetGroup
{
	std::string path;
	std::string outputFile;
	std::string intDir;
	StringList children;
	StringList sources;
	StringList headers;
	StringList dependencies;
	SourceKind sourceKind = SourceKind::None;
	PBXGroupKind kind = PBXGroupKind::Script;
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
	const auto& inputFile = firstState.inputs.inputFile();

	auto rootBuildFile = fmt::format("{}/{}", workingDirectory, inputFile);
	if (!Commands::pathExists(rootBuildFile))
		rootBuildFile = inputFile;

	m_exportPath = String::getPathFolder(String::getPathFolder(inFilename));

	m_projectUUID = Uuid::v5(fmt::format("{}_PBXPROJ", workspaceName), m_xcodeNamespaceGuid);
	m_projectGuid = m_projectUUID.str();

	std::map<std::string, SourceTargetGroup> groups;
	std::map<std::string, std::vector<const IBuildTarget*>> configToTargets;

	for (auto& state : m_states)
	{
		const auto& configName = state->configuration.name();
		if (configToTargets.find(configName) == configToTargets.end())
			configToTargets.emplace(configName, std::vector<const IBuildTarget*>{});

		StringList lastDependencies;
		bool lastDependencyWasSource = false;
		for (auto& target : state->targets)
		{
			configToTargets[configName].push_back(target.get());

			if (target->isSources())
			{
				const auto& sourceTarget = static_cast<const SourceTarget&>(*target);
				state->paths.setBuildDirectoriesBasedOnProjectKind(sourceTarget);

				// const auto& buildSuffix = sourceTarget.buildSuffix();

				const auto& name = sourceTarget.name();
				if (groups.find(name) == groups.end())
				{
					groups.emplace(name, SourceTargetGroup{});
					groups[name].path = workingDirectory;
					groups[name].outputFile = sourceTarget.outputFile();
					groups[name].sourceKind = sourceTarget.kind();
					groups[name].kind = PBXGroupKind::Source;
				}

				// groups[name].intDir = state->paths.intermediateDir(sourceTarget);

				auto& pch = sourceTarget.precompiledHeader();

				if (!lastDependencyWasSource && !lastDependencies.empty())
				{
					for (auto& dep : lastDependencies)
						List::addIfDoesNotExist(groups[name].dependencies, dep);
				}

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

				lastDependencies.clear();
			}
			else
			{
				TargetAdapterPBXProj adapter(*state, *target);
				auto command = adapter.getCommand();
				if (!command.empty())
				{
					auto& name = target->name();
					if (groups.find(name) == groups.end())
					{
						groups.emplace(name, SourceTargetGroup{});
					}

					// Fix an edge case where arches need to be quoted in makefile
					auto& arches = state->inputs.universalArches();
					if (target->isCMake() && !arches.empty())
					{
						auto archString = String::join(arches, ';');
						auto define = "-DCMAKE_OSX_ARCHITECTURES";
						String::replaceAll(command, fmt::format("{}={}", define, archString), fmt::format("{}=\"{}\"", define, archString));
					}

					groups[name].kind = PBXGroupKind::Script;
					groups[name].sources.push_back(std::move(command));
					groups[name].children = adapter.getFiles();

					if (!lastDependencies.empty())
					{
						for (auto& dep : lastDependencies)
							List::addIfDoesNotExist(groups[name].dependencies, dep);

						lastDependencies.clear();
					}
				}
			}

			lastDependencies.emplace_back(target->name());
			lastDependencyWasSource = target->isSources();
		}
	}

	{
		SourceTargetGroup buildAllGroup;
		buildAllGroup.kind = PBXGroupKind::BuildAll;
		buildAllGroup.path = workingDirectory;
		buildAllGroup.children.emplace_back(rootBuildFile);
		for (const auto& [target, _] : groups)
			buildAllGroup.dependencies.emplace_back(target);

		auto name = getAllTargetName();
		groups.emplace(name, std::move(buildAllGroup));
	}

	for (auto& [name, group] : groups)
	{
		if (group.kind == PBXGroupKind::Source)
		{
			std::sort(group.children.begin(), group.children.end());
			std::sort(group.sources.begin(), group.sources.end());
			std::sort(group.headers.begin(), group.headers.end());
		}
		else if (group.kind == PBXGroupKind::Script)
		{
			std::string makefileContents;
			size_t index = 0;
			for (auto& [configName, _] : configToTargets)
			{
				auto& source = group.sources.at(index);
				auto split = String::split(source, '\n');
				makefileContents += fmt::format(R"shell({}:
	@{}

)shell",
					configName,
					String::join(split, "\n\t@"));
				index++;
			}

			auto outPath = fmt::format("{}/scripts/{}.make", m_exportPath, name);
			Commands::createFileWithContents(outPath, makefileContents);
		}
	}

	OldPListGenerator pbxproj;
	pbxproj["archiveVersion"] = 1;
	pbxproj["classes"] = Json::array();
	pbxproj["objectVersion"] = kMinimumObjectVersion;
	pbxproj["objects"] = Json::object();
	auto& objects = pbxproj.at("objects");

	auto mainGroup = Uuid::v5("mainGroup", m_xcodeNamespaceGuid).toAppleHash();
	const std::string group{ "SOURCE_ROOT" };

	// PBXAggregateTarget
	{
		const std::string section{ "PBXAggregateTarget" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (const auto& [target, pbxGroup] : groups)
		{
			if (pbxGroup.kind == PBXGroupKind::Source)
				continue;

			auto key = getTargetHashWithLabel(target);
			node[key]["isa"] = section;
			node[key]["buildConfigurationList"] = getHashWithLabel(getBuildConfigurationListLabel(target, ListType::AggregateTarget));
			node[key]["buildPhases"] = Json::array();
			if (pbxGroup.kind == PBXGroupKind::Script)
			{
				auto phase = getHashWithLabel(target);
				node[key]["buildPhases"].push_back(phase);
			}
			node[key]["dependencies"] = Json::array();
			for (auto& dependency : pbxGroup.dependencies)
			{
				node[key]["dependencies"].push_back(getSectionKeyForTarget("PBXTargetDependency", dependency));
			}
			node[key]["name"] = target;
			node[key]["productName"] = target;
		}
	}

	// PBXBuildFile
	{
		const std::string section{ "PBXBuildFile" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (const auto& [target, pbxGroup] : groups)
		{
			if (pbxGroup.kind == PBXGroupKind::Source)
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
			else
			{
				for (auto& file : pbxGroup.children)
				{
					auto filename = getSourceWithSuffix(file, target);
					auto key = getHashWithLabel(fmt::format("{} in Resources", filename));
					node[key]["isa"] = section;
					node[key]["fileRef"] = getHashedJsonValue(getSourceWithSuffix(file, target));
				}
			}
		}
	}

	// PBXBuildStyle
	{
		const std::string section{ "PBXBuildStyle" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (auto& [configName, _] : configToTargets)
		{
			auto key = getSectionKeyForTarget(configName, configName);
			node[key]["isa"] = section;
			node[key]["buildSettings"] = Json::object();
			node[key]["buildSettings"]["COPY_PHASE_STRIP"] = getBoolString(false);
			node[key]["name"] = configName;
		}
	}

	// PBXFileReference
	{
		const std::string section{ "PBXFileReference" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		struct ProjectFileSet
		{
			std::string file;
			std::string fileType;
		};
		std::map<std::string, ProjectFileSet> projectFileList;
		for (const auto& [target, pbxGroup] : groups)
		{
			if (pbxGroup.kind != PBXGroupKind::Source)
				continue;

			for (auto& file : pbxGroup.sources)
			{
				auto name = getSourceWithSuffix(file, target);
				auto type = firstState.paths.getSourceType(file);
				if (projectFileList.find(name) == projectFileList.end())
					projectFileList.emplace(name, ProjectFileSet{ file, getXcodeFileType(type) });
			}
		}
		for (const auto& [target, pbxGroup] : groups)
		{
			if (pbxGroup.kind != PBXGroupKind::Source)
				continue;

			for (auto& file : pbxGroup.headers)
			{
				auto name = getSourceWithSuffix(file, target);
				auto type = getXcodeFileTypeFromHeader(file);

				if (projectFileList.find(name) == projectFileList.end())
					projectFileList.emplace(name, ProjectFileSet{ file, type });
			}
		}

		for (auto& [name, set] : projectFileList)
		{
			auto key = getHashWithLabel(name);
			node[key]["isa"] = section;
			node[key]["explicitFileType"] = set.fileType;
			node[key]["fileEncoding"] = PBXFileEncoding::UTF8; // assume UTF-8 for now
			node[key]["name"] = String::getPathFilename(set.file);
			node[key]["path"] = set.file;
			node[key]["sourceTree"] = "SOURCE_ROOT";
		}

		//<group>
		for (const auto& [target, pbxGroup] : groups)
		{
			if (pbxGroup.kind == PBXGroupKind::Source)
			{
				auto key = getHashWithLabel(target);
				node[key]["isa"] = section;
				node[key]["explicitFileType"] = getXcodeFileType(pbxGroup.sourceKind);
				node[key]["includeInIndex"] = 0;
				node[key]["path"] = pbxGroup.outputFile;
				node[key]["sourceTree"] = "BUILT_PRODUCTS_DIR";
			}
			else
			{
				for (auto& file : pbxGroup.children)
				{
					auto name = getSourceWithSuffix(file, target);
					auto key = getHashWithLabel(name);
					node[key]["isa"] = section;
					node[key]["explicitFileType"] = getXcodeFileTypeFromFile(file);
					node[key]["includeInIndex"] = 0;
					node[key]["name"] = String::getPathFilename(file);
					node[key]["path"] = file;
					node[key]["sourceTree"] = "SOURCE_ROOT";
				}
			}
		}
	}

	// PBXContainerItemProxy
	{
		const std::string section{ "PBXContainerItemProxy" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (const auto& [target, pbxGroup] : groups)
		{
			auto hash = getTargetHash(target);
			auto key = getSectionKeyForTarget(section, target);
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
			auto label = pbxGroup.kind == PBXGroupKind::BuildAll ? "Build" : "Sources";
			auto key = getHashWithLabel(fmt::format("{} [{}]", label, target));
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

		//
		auto products = getHashWithLabel("Products");
		node[products] = Json::object();
		node[products]["isa"] = section;
		node[products]["children"] = Json::array();
		for (const auto& [target, pbxGroup] : groups)
		{
			if (pbxGroup.kind != PBXGroupKind::Source)
				continue;

			node[products]["children"].push_back(getHashWithLabel(target));
		}
		node[products]["children"].push_back(getHashWithLabel(getAllTargetName()));
		node[products]["name"] = "Products";
		node[products]["sourceTree"] = group;

		//
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

		for (auto& [target, pbxGroup] : groups)
		{
			auto key = getSectionKeyForTarget(section, target);
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
			if (pbxGroup.kind != PBXGroupKind::Source)
				continue;

			auto sources = getSectionKeyForTarget("Sources", target);
			auto resources = getSectionKeyForTarget("Resources", target);
			auto key = getTargetHashWithLabel(target);
			node[key]["isa"] = section;
			node[key]["buildConfigurationList"] = getHashWithLabel(getBuildConfigurationListLabel(target, ListType::NativeProject));
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
			node[key]["productType"] = getNativeProductType(pbxGroup.sourceKind);
		}
	}

	// PBXProject
	{
		const std::string section{ "PBXProject" };
		const std::string region{ "en" };
		auto name = getProjectName();
		objects[section] = Json::object();
		auto& node = objects.at(section);
		auto key = getHashWithLabel(m_projectUUID, "Project object");
		node[key]["isa"] = section;
		node[key]["attributes"] = Json::object();
		node[key]["attributes"]["BuildIndependentTargetsInParallel"] = getBoolString(true);
		node[key]["attributes"]["LastUpgradeCheck"] = 1430;
		// node[key]["attributes"]["TargetAttributes"] = Json::object();
		node[key]["buildConfigurationList"] = getHashWithLabel(getBuildConfigurationListLabel(name, ListType::Project));
		node[key]["buildSettings"] = Json::object();
		node[key]["buildStyles"] = Json::array();
		for (auto& [configName, _] : configToTargets)
		{
			node[key]["buildStyles"].push_back(getSectionKeyForTarget(configName, configName));
		}

		// match version specified in kMinimumObjectVersion
		node[key]["compatibilityVersion"] = "Xcode 3.2";

		node[key]["developmentRegion"] = region;
		node[key]["hasScannedForEncodings"] = 0;
		node[key]["knownRegions"] = {
			"Base",
			region,
		};
		node[key]["mainGroup"] = mainGroup;
		node[key]["projectDirPath"] = workingDirectory;
		node[key]["projectRoot"] = "";
		node[key]["targets"] = Json::array();
		for (const auto& [target, pbxGroup] : groups)
		{
			node[key]["targets"].push_back(getTargetHashWithLabel(target));
		}
	}

	// PBXShellScriptBuildPhase
	{
		const std::string section{ "PBXShellScriptBuildPhase" };
		objects[section] = Json::object();
		auto& node = objects.at(section);

		for (const auto& [target, pbxGroup] : groups)
		{
			if (pbxGroup.kind == PBXGroupKind::Source)
				continue;

			if (!pbxGroup.sources.empty())
			{
				auto makefilePath = fmt::format("{}/scripts/{}.make", m_exportPath, target);
				auto shellScript = fmt::format(R"shell(set -e
echo "*== script start ==*"
make -f {} --no-builtin-rules --no-builtin-variables --no-print-directory $CONFIGURATION
echo "*== script end ==*"
)shell",
					makefilePath);

				auto key = getHashWithLabel(target);
				node[key]["isa"] = section;
				node[key]["alwaysOutOfDate"] = 1;
				node[key]["buildActionMask"] = kBuildActionMask;
				node[key]["files"] = Json::array();
				for (auto& filename : pbxGroup.children)
				{
					node[key]["files"].push_back(getHashWithLabel(fmt::format("{} in Resources", filename)));
				}
				node[key]["inputPaths"] = Json::array();
				node[key]["name"] = target;
				node[key]["outputPaths"] = Json::array();
				node[key]["runOnlyForDeploymentPostprocessing"] = 0;
				node[key]["shellPath"] = "/bin/sh";
				node[key]["shellScript"] = shellScript;
				node[key]["showEnvVarsInLog"] = 0;
			}
		}
	}

	// PBXResourcesBuildPhase
	{
		const std::string section{ "PBXResourcesBuildPhase" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (const auto& [target, pbxGroup] : groups)
		{
			if (pbxGroup.kind != PBXGroupKind::Source)
				continue;

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
			if (pbxGroup.kind != PBXGroupKind::Source)
				continue;

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

	// pbxproj.dumpToTerminal();

	// XCBuildConfiguration
	{
		const std::string section{ "XCBuildConfiguration" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (auto& state : m_states)
		{
			auto& configName = state->configuration.name();
			auto hash = getConfigurationHash(configName);
			auto key = getHashWithLabel(hash, configName);
			node[key]["isa"] = section;
			node[key]["buildSettings"] = getProductBuildSettings(*state);
			// node[key]["baseConfigurationReference"] = "";
			node[key]["name"] = configName;
		}
		for (auto& state : m_states)
		{
			const auto& configName = state->configuration.name();
			for (auto& target : state->targets)
			{
				auto hash = getTargetConfigurationHash(configName, target->name());
				auto key = getHashWithLabel(hash, configName);
				node[key]["isa"] = section;
				if (target->isSources())
					node[key]["buildSettings"] = getBuildSettings(*state, static_cast<const SourceTarget&>(*target));
				else
					node[key]["buildSettings"] = getGenericBuildSettings(*state, *target);
				// node[key]["baseConfigurationReference"] = "";
				node[key]["name"] = configName;
			}
		}
	}

	// XCConfigurationList
	{
		auto project = getProjectName();
		const std::string section{ "XCConfigurationList" };
		objects[section] = Json::object();

		StringList configurations;
		for (auto& [configName, _] : configToTargets)
		{
			auto hash = getConfigurationHash(configName);
			configurations.emplace_back(getHashWithLabel(hash, configName));
		}

		auto& node = objects.at(section);
		{
			auto key = getHashWithLabel(getBuildConfigurationListLabel(project, ListType::Project));
			node[key]["isa"] = section;
			node[key]["buildConfigurations"] = configurations;
			node[key]["defaultConfigurationIsVisible"] = 0;
			node[key]["defaultConfigurationName"] = firstState.configuration.name();
		}

		for (const auto& [target, pbxGroup] : groups)
		{
			configurations.clear();
			for (auto& [configName, targetPtr] : configToTargets)
			{
				auto hash = getTargetConfigurationHash(configName, target);
				configurations.emplace_back(getHashWithLabel(hash, configName));
			}

			auto type = pbxGroup.kind == PBXGroupKind::Source ? ListType::NativeProject : ListType::AggregateTarget;
			auto key = getHashWithLabel(getBuildConfigurationListLabel(target, type));
			node[key]["isa"] = section;
			node[key]["buildConfigurations"] = configurations;
			node[key]["defaultConfigurationIsVisible"] = 0;
			node[key]["defaultConfigurationName"] = firstState.configuration.name();
		}
	}

	pbxproj["rootObject"] = getHashedJsonValue(m_projectUUID, "Project object");

	// pbxproj.dumpToTerminal();

	auto contents = pbxproj.getContents({
		"PBXBuildFile",
		"PBXFileReference",
	});
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
Uuid XcodePBXProjGen::getTargetHash(const std::string& inTarget) const
{
	return Uuid::v5(fmt::format("{}_TARGET", inTarget), m_xcodeNamespaceGuid);
}

/*****************************************************************************/
Uuid XcodePBXProjGen::getConfigurationHash(const std::string& inConfig) const
{
	return Uuid::v5(fmt::format("{}_PROJECT", inConfig), m_xcodeNamespaceGuid);
}

/*****************************************************************************/
Uuid XcodePBXProjGen::getTargetConfigurationHash(const std::string& inConfig, const std::string& inTarget) const
{
	return Uuid::v5(fmt::format("{}-{}", inConfig, inTarget), m_xcodeNamespaceGuid);
}

/*****************************************************************************/
std::string XcodePBXProjGen::getTargetHashWithLabel(const std::string& inTarget) const
{
	return getHashWithLabel(getTargetHash(inTarget), inTarget);
}

/*****************************************************************************/
std::string XcodePBXProjGen::getSectionKeyForTarget(const std::string& inKey, const std::string& inTarget) const
{
	return getHashWithLabel(Uuid::v5(fmt::format("{}_KEY [{}]", inKey, inTarget), m_xcodeNamespaceGuid), inKey);
}

/*****************************************************************************/
std::string XcodePBXProjGen::getBuildConfigurationListLabel(const std::string& inName, const ListType inType) const
{
	const char* type;
	if (inType == ListType::NativeProject)
		type = "PBXNativeTarget";
	else if (inType == ListType::AggregateTarget)
		type = "PBXAggregateTarget";
	else
		type = "PBXProject";

	return fmt::format("Build configuration list for {} \"{}\"", type, inName);
}

/*****************************************************************************/
std::string XcodePBXProjGen::getAllTargetName() const
{
	return std::string("[all]");
}

/*****************************************************************************/
std::string XcodePBXProjGen::getProjectName() const
{
	// const auto& firstState = *m_states.front();
	// const auto& workspaceName = firstState.workspace.metadata().name();
	// return !workspaceName.empty() ? workspaceName : std::string("project");

	return std::string("project");
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
	return Json(getHashWithLabel(inHash, inLabel));
}

/*****************************************************************************/
std::string XcodePBXProjGen::getBoolString(const bool inValue) const
{
	return inValue ? "YES" : "NO";
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
			return "sourcecode.cpp.cpp";
		default:
			return "automatic";
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
std::string XcodePBXProjGen::getXcodeFileTypeFromHeader(const std::string& inFile) const
{
	auto ext = String::getPathSuffix(inFile);
	if (String::equals('h', ext))
		return "sourcecode.c.h";
	else
		return "sourcecode.cpp.h";
}

/*****************************************************************************/
std::string XcodePBXProjGen::getXcodeFileTypeFromFile(const std::string& inFile) const
{
	auto ext = String::getPathSuffix(inFile);
	if (ext.empty())
		return "automatic";

	if (String::equals("txt", ext))
		return "text";
	else if (String::equals("json", ext))
		return "text.json";
	else if (String::equals("storyboard", ext))
		return "file.storyboard";
	else if (String::equals({ "png", "gif", "jpg" }, ext))
		return "image";
	// Source code (easy)
	else if (String::equals("c", ext))
		return "sourcecode.c.c";
	else if (String::equals("mm", ext))
		return "sourcecode.cpp.objcpp";
	else if (String::equals("swift", ext))
		return "sourcecode.swift";
	else if (String::equals("plist", ext))
		return "sourcecode.text.plist";
	else if (String::equals("h", ext))
		return "sourcecode.c.h";
	else if (String::equals("asm", ext))
		return "sourcecode.asm";
	else if (String::equals("metal", ext))
		return "sourcecode.metal";
	else if (String::equals("mig", ext))
		return "sourcecode.mig";
	else if (String::equals("tbd", ext))
		return "sourcecode.text-based-dylib-definition";
	// Apple
	else if (String::equals("xctest", ext))
		return "wrapper.cfbundle";
	else if (String::equals("framework", ext))
		return "wrapper.framework";
	else if (String::equals("xcassets", ext))
		return "folder.assetcatalog";
	else if (String::equals("xcconfig", ext))
		return "text.xcconfig";
	else if (String::equals("xib", ext))
		return "file.xib";
	// Compiled
	else if (String::equals("a", ext))
		return "archive.ar";
	else if (String::equals("o", ext))
		return "compiled.mach-o.objfile";
	else if (String::equals("dylib", ext))
		return "compiled.mach-o.dylib";
	// Source code (complex)
	else if (String::equals({ "m", "M" }, ext))
		return "sourcecode.c.objc";
	else if (String::equals({ "hpp", "hh", "hxx", "H", "inl", "ii", "ixx", "h++", "ipp", "txx", "tpp", "tpl" }, ext))
		return "sourcecode.cpp.h";
	else if (String::equals({ "cpp", "cc", "cxx", "C", "c++", "cppm" }, ext))
		return "sourcecode.cpp.cpp";
	else if (String::equals({ "for", "f90", "f" }, ext))
		return "sourcecode.fortran.f90";

	return "automatic";
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
std::string XcodePBXProjGen::getSourceWithSuffix(const std::string& inFile, const std::string& inSuffix) const
{
	return fmt::format("[{}] {}", inSuffix, inFile);
}

/*****************************************************************************/
Json XcodePBXProjGen::getBuildSettings(BuildState& inState, const SourceTarget& inTarget) const
{
	const auto& config = inState.configuration;

	CommandAdapterClang clangAdapter(inState, inTarget);

	const auto& cwd = inState.inputs.workingDirectory();
	auto lang = inTarget.language();
	inState.paths.setBuildDirectoriesBasedOnProjectKind(inTarget);

	// TODO: this is currently just based on a Release mode

	auto buildDir = fmt::format("{}/{}", cwd, inState.paths.outputDirectory());
	auto buildOutputDir = fmt::format("{}/{}", cwd, inState.paths.buildOutputDir());
	auto objectDirectory = fmt::format("{}/obj.{}", buildOutputDir, inTarget.name());

	Json ret;

	ret["ALWAYS_SEARCH_USER_PATHS"] = getBoolString(false);
	ret["BUILD_DIR"] = buildDir;
	ret["CLANG_ANALYZER_NONNULL"] = getBoolString(true);
	ret["CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION"] = "YES_AGGRESSIVE";

	auto cStandard = clangAdapter.getLanguageStandardC();
	auto cppStandard = clangAdapter.getLanguageStandardCpp();

	if (!cppStandard.empty())
		ret["CLANG_CXX_LANGUAGE_STANDARD"] = std::move(cppStandard);

	ret["CLANG_CXX_LIBRARY"] = clangAdapter.getCxxLibrary();

	ret["CLANG_ENABLE_MODULES"] = getBoolString(inTarget.objectiveCxx());

	if (inTarget.objectiveCxx())
	{
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

	ret["CLANG_WARN_DOCUMENTATION_COMMENTS"] = getBoolString(false);
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
	ret["COMBINE_HIDPI_IMAGES"] = getBoolString(true);

	// auto& signingIdentity = inState.tools.signingIdentity();
	std::string signingIdentity;
	ret["CODE_SIGN_IDENTITY"] = !signingIdentity.empty() ? signingIdentity : "-";
	ret["CODE_SIGN_INJECT_BASE_ENTITLEMENTS"] = getBoolString(false);

	ret["CONFIGURATION_BUILD_DIR"] = buildOutputDir;
	ret["CONFIGURATION_TEMP_DIR"] = objectDirectory;
	ret["COPY_PHASE_STRIP"] = getBoolString(false);
	if (config.debugSymbols())
	{
		ret["DEBUG_INFORMATION_FORMAT"] = "dwarf-with-dsym";
	}
	// ret["ENABLE_NS_ASSERTIONS"] = getBoolString(false);

	// ret["DERIVED_FILE_DIR"] = objectDirectory; // $(CONFIGURATION_TEMP_DIR)/DerivedSources dir

	// include dirs
	{
		StringList searchPaths;
		const auto& includeDirs = inTarget.includeDirs();
		const auto& objDir = inState.paths.objDir();
		const auto& externalBuildDir = inState.paths.externalBuildDir();
		const auto& intDir = inState.paths.intermediateDir(inTarget);
		for (auto& include : includeDirs)
		{
			if (String::equals(objDir, include))
			{
				searchPaths.emplace_back(objectDirectory);
			}
			else if (String::startsWith(externalBuildDir, include))
			{
				searchPaths.emplace_back(fmt::format("{}/{}", cwd, include));
			}
			else
			{
				auto temp = fmt::format("{}/{}", cwd, include);
				if (String::equals(intDir, include) || Commands::pathExists(temp))
					searchPaths.emplace_back(std::move(temp));
				else
					searchPaths.emplace_back(include);
			}
		}

		searchPaths.emplace_back("$(inherited)");
		ret["HEADER_SEARCH_PATHS"] = std::move(searchPaths);
	}

	if (inTarget.objectiveCxx())
	{
		ret["ENABLE_STRICT_OBJC_MSGSEND"] = getBoolString(true);
	}
	ret["ENABLE_TESTABILITY"] = getBoolString(true);

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

	ret["GENERATE_PROFILING_CODE"] = getBoolString(config.enableProfiling());

	if (inTarget.usesPrecompiledHeader())
	{
		ret["GCC_PREFIX_HEADER"] = fmt::format("{}/{}", cwd, inTarget.precompiledHeader());
		ret["GCC_PRECOMPILE_PREFIX_HEADER"] = getBoolString(true);
	}

	if (!cStandard.empty())
		ret["GCC_C_LANGUAGE_STANDARD"] = std::move(cStandard);

	ret["GCC_ENABLE_CPP_EXCEPTIONS"] = getBoolString(clangAdapter.supportsExceptions());
	ret["GCC_ENABLE_CPP_RTTI"] = getBoolString(clangAdapter.supportsRunTimeTypeInformation());

	ret["GCC_GENERATE_DEBUGGING_SYMBOLS"] = getBoolString(config.debugSymbols());
	ret["GCC_NO_COMMON_BLOCKS"] = getBoolString(true);
	ret["GCC_OPTIMIZATION_LEVEL"] = clangAdapter.getOptimizationLevel();

	ret["GCC_PREPROCESSOR_DEFINITIONS"] = inTarget.defines();
	ret["GCC_PREPROCESSOR_DEFINITIONS"].push_back("$(inherited)");

	ret["GCC_TREAT_WARNINGS_AS_ERRORS"] = getBoolString(inTarget.treatWarningsAsErrors());

	ret["GCC_WARN_64_TO_32_BIT_CONVERSION"] = getBoolString(true);
	ret["GCC_WARN_ABOUT_RETURN_TYPE"] = "YES_ERROR";
	ret["GCC_WARN_UNDECLARED_SELECTOR"] = getBoolString(true);
	ret["GCC_WARN_UNINITIALIZED_AUTOS"] = "YES_AGGRESSIVE";
	ret["GCC_WARN_UNUSED_FUNCTION"] = getBoolString(true);
	ret["GCC_WARN_UNUSED_VARIABLE"] = getBoolString(true);

	// lib dirs & Runpath search paths
	{
		StringList runPaths;
		StringList searchPaths;
		const auto& libDirs = inTarget.libDirs();
		const auto& workspaceSearchPaths = inState.workspace.searchPaths();
		const auto& objDir = inState.paths.objDir();
		const auto& externalDir = inState.inputs.externalDirectory();
		const auto& externalBuildDir = inState.paths.externalBuildDir();
		const auto& intDir = inState.paths.intermediateDir(inTarget);
		const auto& appleFrameworkPaths = inTarget.appleFrameworkPaths();

		for (auto& libDir : libDirs)
		{
			if (String::equals(objDir, libDir))
			{
				searchPaths.emplace_back(objectDirectory);
			}
			else if (String::startsWith(externalBuildDir, libDir) || String::startsWith(externalDir, libDir))
			{
				auto temp = fmt::format("{}/{}", cwd, libDir);
				runPaths.emplace_back(temp);
				searchPaths.emplace_back(temp);
			}
			else
			{
				auto temp = fmt::format("{}/{}", cwd, libDir);
				if (String::equals(intDir, libDir) || Commands::pathExists(temp))
				{
					runPaths.emplace_back(temp);
					searchPaths.emplace_back(temp);
				}
				else
				{
					runPaths.emplace_back(libDir);
					searchPaths.emplace_back(libDir);
				}
			}
		}
		for (auto& path : appleFrameworkPaths)
		{
			if (String::startsWith(externalBuildDir, path) || String::startsWith(externalDir, path))
			{
				auto temp = fmt::format("{}/{}", cwd, path);
				runPaths.emplace_back(temp);
			}
			else
			{
				runPaths.emplace_back(path);
			}
		}
		for (auto& path : workspaceSearchPaths)
		{
			runPaths.emplace_back(path);
		}

		runPaths.emplace_back("$(inherited)");
		ret["LD_RUNPATH_SEARCH_PATHS"] = std::move(runPaths);

		searchPaths.emplace_back("$(inherited)");
		ret["LIBRARY_SEARCH_PATHS"] = std::move(searchPaths);
	}

	// YES, YES_THIN, NO
	//   TODO: thin = incremental - maybe add in the future?
	// ret["LLVM_LTO"] = getBoolString(config.interproceduralOptimization());
	ret["LLVM_LTO"] = config.interproceduralOptimization() ? "YES_THIN" : "NO";

	ret["MACH_O_TYPE"] = getMachOType(inTarget);
	ret["MACOSX_DEPLOYMENT_TARGET"] = inState.inputs.osTargetVersion();
	ret["MTL_ENABLE_DEBUG_INFO"] = getBoolString(inState.configuration.debugSymbols());
	ret["MTL_FAST_MATH"] = getBoolString(clangAdapter.supportsFastMath());
	ret["OBJECT_FILE_DIR"] = objectDirectory;
	ret["OBJROOT"] = buildOutputDir;
	ret["ONLY_ACTIVE_ARCH"] = getBoolString(false);

	auto compileOptions = getCompilerOptions(inState, inTarget);
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

	auto linkerOptions = getLinkerOptions(inState, inTarget);
	if (!linkerOptions.empty())
	{
		ret["OTHER_LDFLAGS"] = linkerOptions;
	}

	// TODO: Generate .app target based on the distribution
	// ret["PRODUCT_BUNDLE_IDENTIFIER"] = "com.developer.application";

	ret["PRODUCT_NAME"] = "$(TARGET_NAME)";
	ret["PROJECT_RUN_PATH"] = cwd;
	ret["SDKROOT"] = inState.tools.getApplePlatformSdk(inState.inputs.osTargetName());
	// ret["SYMROOT"] = buildOutputDir;
	ret["SHARED_PRECOMPS_DIR"] = buildOutputDir;
	ret["TARGET_TEMP_DIR"] = objectDirectory;
	ret["USE_HEADERMAP"] = getBoolString(false);

	// ret["BUILD_ROOT"] = fmt::format("{}/{}", cwd, inState.paths.buildOutputDir());
	// ret["SWIFT_OBJC_BRIDGING_HEADER"] = "";

	// Code signing
	// ret["CODE_SIGN_STYLE"] = "Automatic";
	// ret["DEVELOPMENT_TEAM"] = ""; // required!

	return ret;
}

/*****************************************************************************/
Json XcodePBXProjGen::getGenericBuildSettings(BuildState& inState, const IBuildTarget& inTarget) const
{
	UNUSED(inTarget);
	const auto& cwd = inState.inputs.workingDirectory();

	// TODO: this is currently just based on a Release mode

	// auto buildDir = fmt::format("{}/{}", cwd, inState.paths.outputDirectory());
	auto buildOutputDir = fmt::format("{}/{}", cwd, inState.paths.buildOutputDir());
	auto objectDirectory = fmt::format("{}/obj.{}", buildOutputDir, inTarget.name());

	Json ret;

	ret["ALWAYS_SEARCH_USER_PATHS"] = getBoolString(false);
	// ret["BUILD_DIR"] = buildDir;

	ret["CONFIGURATION_BUILD_DIR"] = buildOutputDir;
	ret["CONFIGURATION_TEMP_DIR"] = objectDirectory;
	ret["OBJECT_FILE_DIR"] = objectDirectory;
	ret["OBJROOT"] = buildOutputDir;
	ret["PROJECT_RUN_PATH"] = cwd;
	ret["SHARED_PRECOMPS_DIR"] = buildOutputDir;
	ret["SDKROOT"] = inState.tools.getApplePlatformSdk(inState.inputs.osTargetName());
	ret["TARGET_TEMP_DIR"] = objectDirectory;
	// ret["SYMROOT"] = buildOutputDir;
	// ret["USE_HEADERMAP"] = getBoolString(false);

	// ret["BUILD_ROOT"] = fmt::format("{}/{}", cwd, inState.paths.buildOutputDir());
	// ret["SWIFT_OBJC_BRIDGING_HEADER"] = "";

	return ret;
}

/*****************************************************************************/
Json XcodePBXProjGen::getProductBuildSettings(const BuildState& inState) const
{
	Json ret;

	const auto& cwd = inState.inputs.workingDirectory();
	auto buildOutputDir = fmt::format("{}/{}", cwd, inState.paths.buildOutputDir());

	auto arches = inState.inputs.universalArches();
	if (arches.empty())
		ret["ARCHS"] = inState.info.targetArchitectureString();

	ret["PROJECT_RUN_PATH"] = cwd;
	ret["SDKROOT"] = inState.tools.getApplePlatformSdk(inState.inputs.osTargetName());
	// ret["SYMROOT"] = buildOutputDir;

	return ret;
}

/*****************************************************************************/
StringList XcodePBXProjGen::getCompilerOptions(const BuildState& inState, const SourceTarget& inTarget) const
{
	UNUSED(inState);
	CommandAdapterClang clangAdapter(inState, inTarget);

	StringList ret;

	// Coroutines
	if (clangAdapter.supportsCppCoroutines())
		ret.emplace_back("-fcoroutines-ts");

	// Concepts
	if (clangAdapter.supportsCppConcepts())
		ret.emplace_back("-fconcepts-ts");

	//  Warnings
	auto warnings = clangAdapter.getWarningList();
	for (auto& warning : warnings)
	{
		if (String::equals("pedantic-errors", warning))
			ret.emplace_back(fmt::format("-{}", warning));
		else
			ret.emplace_back(fmt::format("-W{}", warning));
	}

	// Charsets
	auto inputCharset = String::toUpperCase(inTarget.inputCharset());
	ret.emplace_back(fmt::format("-finput-charset={}", inputCharset));

	auto execCharset = String::toUpperCase(inTarget.executionCharset());
	ret.emplace_back(fmt::format("-fexec-charset={}", execCharset));

	// Position Independent Code
	if (inTarget.positionIndependentCode())
		ret.emplace_back("-fPIC");
	else if (inTarget.positionIndependentExecutable())
		ret.emplace_back("-fPIE");

	// Diagnostic Color
	ret.emplace_back("-fdiagnostics-color=always");

	// User Compile Options
	auto& compileOptions = inTarget.compileOptions();
	for (auto& option : compileOptions)
		List::addIfDoesNotExist(ret, option);

	// Fast Math
	// if (clangAdapter.supportsFastMath())
	// 	List::addIfDoesNotExist(ret, "-ffast-math");

	// RTTI
	// if (!clangAdapter.supportsRunTimeTypeInformation())
	// 	List::addIfDoesNotExist(ret, "-fno-rtti");

	// Exceptions
	// if (!clangAdapter.supportsExceptions())
	// 	List::addIfDoesNotExist(ret, "-fno-exceptions");

	// Thread Model
	if (inTarget.threads())
		List::addIfDoesNotExist(ret, "-pthread");

	// Sanitizers
	StringList sanitizers = clangAdapter.getSanitizersList();
	if (!sanitizers.empty())
	{
		auto list = String::join(sanitizers, ',');
		ret.emplace_back(fmt::format("-fsanitize={}", list));
	}

	return ret;
}

/*****************************************************************************/
StringList XcodePBXProjGen::getLinkerOptions(const BuildState& inState, const SourceTarget& inTarget) const
{
	CommandAdapterClang clangAdapter(inState, inTarget);

	StringList ret;

	// Position Independent Code
	if (inTarget.positionIndependentCode())
		ret.emplace_back("-fPIC");
	else if (inTarget.positionIndependentExecutable())
		ret.emplace_back("-fPIE");

	// User Linker Options
	for (auto& option : inTarget.linkerOptions())
		ret.push_back(option);

	// Thread Model
	if (inTarget.threads())
		List::addIfDoesNotExist(ret, "-pthread");

	// Sanitizers
	StringList sanitizers = clangAdapter.getSanitizersList();
	if (!sanitizers.empty())
	{
		auto list = String::join(sanitizers, ',');
		ret.emplace_back(fmt::format("-fsanitize={}", list));
	}

	// Static Compiler Libraries
	if (inTarget.staticRuntimeLibrary() && inState.configuration.sanitizeAddress())
		List::addIfDoesNotExist(ret, "-static-libsan");

	const auto& links = inTarget.links();
	for (auto& link : links)
	{
		ret.push_back(fmt::format("-l{}", link));
	}
	const auto& staticLinks = inTarget.staticLinks();
	for (auto& link : staticLinks)
	{
		ret.push_back(fmt::format("-l{}", link));
	}

	// Apple Framework Options
	{
		for (auto& path : inTarget.libDirs())
		{
			ret.emplace_back(fmt::format("-F{}", path));
		}
		for (auto& path : inTarget.appleFrameworkPaths())
		{
			ret.emplace_back(fmt::format("-F{}", path));
		}
		List::addIfDoesNotExist(ret, "-F/Library/Frameworks");
	}
	for (auto& framework : inTarget.appleFrameworks())
	{
		ret.emplace_back("-framework");
		ret.push_back(framework);
	}

	return ret;
}
}
