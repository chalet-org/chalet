/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Xcode/XcodePBXProjGen.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Bundler/AppBundlerMacOS.hpp"
#include "Bundler/BinaryDependency/BinaryDependencyMap.hpp"
#include "Compile/CommandAdapter/CommandAdapterClang.hpp"
#include "Compile/CompilerCxx/CompilerCxxGCC.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/TargetExportAdapter.hpp"
#include "Export/Xcode/OldPListGenerator.hpp"
#include "Libraries/Json.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/IDistTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
enum class TargetGroupKind : u16
{
	Source,
	Script,
	BuildAll,
	AppBundle,
};
enum class PBXFileEncoding : u32
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
enum class DstSubfolderSpec : u32
{
	AbsolutePath = 0,
	Wrapper = 1,
	Executables = 6,
	Resources = 7,
	Frameworks = 10,
	SharedFrameworks = 11,
	SharedSupport = 12,
	PluginsAndFoundationExtensions = 13,
	JavaResources = 15,
	Products = 16,
};
struct TargetGroup
{
	std::string path;
	std::string outputFile;
	StringList children;
	StringList sources;
	StringList headers;
	StringList dependencies;
	StringList resources;
	SourceKind targetKind = SourceKind::None;
	TargetGroupKind kind = TargetGroupKind::Script;
};
struct ProjectFileSet
{
	std::string file;
	std::string fileType;
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
constexpr i32 kMinimumObjectVersion = 46;
constexpr i32 kBuildActionMask = 2147483647;

/*****************************************************************************/
XcodePBXProjGen::XcodePBXProjGen(std::vector<Unique<BuildState>>& inStates, const std::string& inAllBuildName) :
	m_states(inStates),
	m_allBuildName(inAllBuildName),
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

	m_generatedBundleFiles.clear();

	const auto& firstState = *m_states.front();
	const auto& workspaceName = firstState.workspace.metadata().name();
	const auto& workingDirectory = firstState.inputs.workingDirectory();
	const auto& inputFile = firstState.inputs.inputFile();

	// const auto& chaletPath = firstState.inputs.appPath();

	auto rootBuildFile = fmt::format("{}/{}", workingDirectory, inputFile);
	if (!Files::pathExists(rootBuildFile))
		rootBuildFile = inputFile;

	m_exportPath = String::getPathFolder(String::getPathFolder(inFilename));

	m_projectUUID = Uuid::v5(fmt::format("{}_PBXPROJ", workspaceName), m_xcodeNamespaceGuid);
	m_projectGuid = m_projectUUID.str();

	std::map<std::string, TargetGroup> groups;
	std::map<std::string, std::vector<const IBuildTarget*>> configToTargets;
	std::map<std::string, StringList> embedLibraries;

	StringList sourceTargets;

	for (auto& state : m_states)
	{
		auto sharedExt = state->environment->getSharedLibraryExtension();
		const auto& configName = state->configuration.name();
		if (configToTargets.find(configName) == configToTargets.end())
			configToTargets.emplace(configName, std::vector<const IBuildTarget*>{});

		for (auto& target : state->targets)
		{
			configToTargets[configName].push_back(target.get());

			if (target->isSources())
			{
				List::addIfDoesNotExist(sourceTargets, target->name());

				const auto& sourceTarget = static_cast<const SourceTarget&>(*target);
				state->paths.setBuildDirectoriesBasedOnProjectKind(sourceTarget);

				const auto& buildOutputDir = state->paths.buildOutputDir();
				// const auto& buildSuffix = sourceTarget.buildSuffix();

				const auto& name = sourceTarget.name();
				if (groups.find(name) == groups.end())
				{
					groups.emplace(name, TargetGroup{});
					groups[name].path = workingDirectory;
					groups[name].outputFile = sourceTarget.outputFile();
					groups[name].targetKind = sourceTarget.kind();
					groups[name].kind = TargetGroupKind::Source;
				}

				auto& pch = sourceTarget.precompiledHeader();

				state->getTargetDependencies(groups[name].dependencies, name, false);

				{
					StringList searches;
					const auto& links = sourceTarget.links();
					for (auto& link : links)
					{
						if (String::endsWith(sharedExt, link))
						{
							if (embedLibraries.find(name) == embedLibraries.end())
								embedLibraries.emplace(name, StringList{});

							// searches.emplace_back(link);

							auto outLink = link;
							String::replaceAll(outLink, buildOutputDir, "$BUILD_OUTPUT_DIR");
							List::addIfDoesNotExist(embedLibraries[name], std::move(outLink));
						}
						else
						{
							searches.emplace_back(fmt::format("/lib{}.dylib", link));
						}
					}
					const auto& appleFrameworks = sourceTarget.appleFrameworks();
					for (auto& framework : appleFrameworks)
					{
						if (Files::pathExists(framework))
							searches.emplace_back(framework);
						else
							searches.emplace_back(fmt::format("/{}.framework", framework));
					}
					const auto& workspaceSearchPaths = state->workspace.searchPaths();

					StringList extensions{
						".dylib",
						".framework",
					};

					StringList libDirs = sourceTarget.libDirs();
					const auto& appleFrameworkPaths = sourceTarget.appleFrameworkPaths();
					for (auto& path : appleFrameworkPaths)
						List::addIfDoesNotExist(libDirs, path);

					for (auto& path : workspaceSearchPaths)
						List::addIfDoesNotExist(libDirs, path);

					for (auto& dir : libDirs)
					{
						if (String::startsWith(buildOutputDir, dir))
							continue;

						if (String::startsWith("/System/Library/Frameworks/", dir))
							continue;

						auto resolvedDir = Files::getCanonicalPath(dir);
						if (!Files::pathExists(resolvedDir))
							continue;

						for (const auto& entry : fs::recursive_directory_iterator(resolvedDir))
						{
							auto path = entry.path().string();
							if (entry.is_regular_file() || String::endsWith(".framework", path))
							{
								if (!String::endsWith(extensions, path))
									continue;

								for (auto& file : searches)
								{
									if (String::endsWith(file, path))
									{
										if (embedLibraries.find(name) == embedLibraries.end())
											embedLibraries.emplace(name, StringList{});

										List::addIfDoesNotExist(embedLibraries[name], std::move(path));
										break;
									}
								}
							}
						}
					}
				}

				const auto& files = sourceTarget.files();
				for (auto& file : files)
				{
					// Just in case
					if (String::endsWith({ ".rc", ".RC" }, file))
						continue;

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
			else
			{
				TargetExportAdapter adapter(*state, *target);
				auto command = adapter.getCommand();
				if (!command.empty())
				{
					auto& name = target->name();
					if (groups.find(name) == groups.end())
					{
						groups.emplace(name, TargetGroup{});
					}

					// Fix an edge case where arches need to be quoted in makefile
					auto& arches = state->inputs.universalArches();
					if (target->isCMake() && !arches.empty())
					{
						auto archString = String::join(arches, ';');
						auto define = "-DCMAKE_OSX_ARCHITECTURES";
						String::replaceAll(command, fmt::format("{}={}", define, archString), fmt::format("{}=\"{}\"", define, archString));
					}

					state->getTargetDependencies(groups[name].dependencies, name, false);
					groups[name].kind = TargetGroupKind::Script;
					groups[name].sources.push_back(std::move(command));
					groups[name].children = adapter.getFiles();
				}
			}
		}

		for (auto& target : state->distribution)
		{
			if (target->isDistributionBundle())
			{
#if defined(CHALET_MACOS)
				auto& bundle = static_cast<BundleTarget&>(*target);
				if (bundle.isMacosAppBundle())
				{
					auto name = bundle.name();
					if (List::contains(sourceTargets, name))
						name += '_';

					if (groups.find(name) == groups.end())
					{
						auto bundleDirectory = fmt::format("{}/{}", m_exportPath, bundle.name());

						groups.emplace(name, TargetGroup{});
						groups[name].kind = TargetGroupKind::AppBundle;
						groups[name].path = workingDirectory;
						groups[name].outputFile = fmt::format("{}.app", target->name());

						auto& icon = bundle.macosBundleIcon();
						bool iconIsIcns = String::endsWith(".icns", icon);
						bool iconIsIconSet = String::endsWith(".iconset", icon);
						bool iconIsBuilt = iconIsIcns || iconIsIconSet;
						if (!icon.empty())
						{
							std::string resolvedIcon;
							if (iconIsIconSet)
							{
								auto iconBaseName = String::getPathBaseName(icon);
								resolvedIcon = fmt::format("{}/{}/{}.icns", m_exportPath, target->name(), iconBaseName);
							}
							else
							{
								resolvedIcon = Files::getCanonicalPath(icon);
							}
							groups[name].children.emplace_back(resolvedIcon);

							if (iconIsBuilt)
								groups[name].resources.emplace_back(std::move(resolvedIcon));
						}

						bool hasXcassets = icon.empty() || (!icon.empty() && !iconIsBuilt);
						if (hasXcassets)
						{
							groups[name].children.emplace_back(fmt::format("{}/Assets.xcassets", bundleDirectory));
							groups[name].resources.emplace_back(fmt::format("{}/Assets.xcassets", bundleDirectory));
						}

						bool hasEntitlements = bundle.willHaveMacosEntitlementsPlist();
						if (hasEntitlements)
							groups[name].children.emplace_back(fmt::format("{}/App.entitlements", bundleDirectory));

						bool hasInfo = bundle.willHaveMacosInfoPlist();
						if (hasInfo)
							groups[name].children.emplace_back(fmt::format("{}/Info.plist", bundleDirectory));

						if (bundle.resolveIncludesFromState(*state))
						{
							auto& includes = bundle.includes();
							for (auto& file : includes)
							{
								groups[name].children.emplace_back(file);
								groups[name].resources.emplace_back(file);
							}
						}

						auto& buildTargets = bundle.buildTargets();
						for (auto& tgt : buildTargets)
						{
							List::addIfDoesNotExist(m_appBuildTargets, tgt);

							if (List::contains(sourceTargets, tgt))
							{
								groups[name].dependencies.emplace_back(tgt);

								if (embedLibraries.find(tgt) != embedLibraries.end())
								{
									auto& embeds = embedLibraries.at(tgt);
									for (auto& embed : embeds)
									{
										List::addIfDoesNotExist(groups[name].headers, embed);
									}
								}
							}
						}
					}
				}
#endif
			}
		}
	}

	{
		TargetGroup buildAllGroup;
		buildAllGroup.kind = TargetGroupKind::BuildAll;
		buildAllGroup.path = workingDirectory;
		buildAllGroup.children.emplace_back(rootBuildFile);
		for (const auto& [target, pbxGroup] : groups)
		{
			if (pbxGroup.kind == TargetGroupKind::AppBundle)
				continue;

			buildAllGroup.dependencies.emplace_back(target);
		}

		groups.emplace(m_allBuildName, std::move(buildAllGroup));
	}

	for (auto& [name, group] : groups)
	{
		if (group.kind == TargetGroupKind::Source)
		{
			std::sort(group.children.begin(), group.children.end());
			std::sort(group.sources.begin(), group.sources.end());
			std::sort(group.headers.begin(), group.headers.end());
		}
		else if (group.kind == TargetGroupKind::Script)
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

			auto outPath = fmt::format("{}/scripts/{}.mk", m_exportPath, Hash::uint64(name));
			Files::createFileWithContents(outPath, makefileContents);
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
			if (pbxGroup.kind == TargetGroupKind::Source || pbxGroup.kind == TargetGroupKind::AppBundle)
				continue;

			auto key = getTargetHashWithLabel(target);
			node[key]["isa"] = section;
			node[key]["buildConfigurationList"] = getHashWithLabel(getBuildConfigurationListLabel(target, ListType::AggregateTarget));
			node[key]["buildPhases"] = Json::array();
			if (pbxGroup.kind == TargetGroupKind::Script)
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
			if (pbxGroup.kind == TargetGroupKind::Source)
			{
				for (auto& file : pbxGroup.sources)
				{
					auto name = getSourceWithSuffix(file, target);
					auto key = getHashWithLabel(fmt::format("{} in Sources", name));
					node[key]["isa"] = section;
					node[key]["fileRef"] = getHashedJsonValue(name);
				}
				for (auto& file : pbxGroup.headers)
				{
					auto name = getSourceWithSuffix(file, target);
					auto key = getHashWithLabel(fmt::format("{} in Sources", name));
					node[key]["isa"] = section;
					node[key]["fileRef"] = getHashedJsonValue(name);
				}
			}
			else
			{
				for (auto& file : pbxGroup.children)
				{
					auto name = getSourceWithSuffix(file, target);
					auto key = getHashWithLabel(fmt::format("{} in Resources", name));
					node[key]["isa"] = section;
					node[key]["fileRef"] = getHashedJsonValue(name);
				}

				if (pbxGroup.kind == TargetGroupKind::AppBundle)
				{
					for (auto& file : pbxGroup.dependencies)
					{
						auto name = getSourceWithSuffix(file, target);
						auto key = getHashWithLabel(fmt::format("{} in CopyFiles", name));
						node[key]["isa"] = section;
						node[key]["fileRef"] = getHashedJsonValue(file);
					}

					for (auto& file : pbxGroup.headers)
					{
						auto name = getSourceWithSuffix(file, target);
						auto key = getHashWithLabel(fmt::format("{} in Embed Libraries", name));
						node[key]["isa"] = section;
						node[key]["fileRef"] = getHashedJsonValue(file);
						node[key]["settings"] = Json::object();
						node[key]["settings"]["ATTRIBUTES"] = "(CodeSignOnCopy, )";
					}
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
		std::map<std::string, ProjectFileSet> projectFileList;
		for (const auto& [target, pbxGroup] : groups)
		{
			if (pbxGroup.kind != TargetGroupKind::Source)
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
			if (pbxGroup.kind == TargetGroupKind::Source || pbxGroup.kind == TargetGroupKind::AppBundle)
			{
				for (auto& file : pbxGroup.headers)
				{
					auto name = getSourceWithSuffix(file, target);
					std::string type;
					if (pbxGroup.kind == TargetGroupKind::AppBundle)
						type = getXcodeFileTypeFromFile(file);
					else
						type = getXcodeFileTypeFromHeader(file);

					if (projectFileList.find(name) == projectFileList.end())
						projectFileList.emplace(name, ProjectFileSet{ file, type });
				}
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
			if (pbxGroup.kind == TargetGroupKind::Source)
			{
				auto key = getHashWithLabel(target);
				node[key]["isa"] = section;
				node[key]["explicitFileType"] = getXcodeFileType(pbxGroup.targetKind);
				node[key]["includeInIndex"] = 0;
				node[key]["path"] = pbxGroup.outputFile;
				node[key]["sourceTree"] = "BUILT_PRODUCTS_DIR";
			}
			else
			{
				bool isBundle = pbxGroup.kind == TargetGroupKind::AppBundle;
				if (isBundle)
				{
					auto filename = String::getPathFilename(pbxGroup.outputFile);
					auto key = getHashWithLabel(target);
					node[key]["isa"] = section;
					node[key]["explicitFileType"] = getXcodeFileTypeFromFile(filename);
					node[key]["includeInIndex"] = 0;
					node[key]["name"] = target;
					node[key]["path"] = pbxGroup.outputFile;
					node[key]["sourceTree"] = "BUILT_PRODUCTS_DIR";
				}

				for (auto& file : pbxGroup.children)
				{
					auto fileType = getXcodeFileTypeFromFile(file);
					bool isDirectory = String::equals("automatic", fileType) && Files::pathIsDirectory(file);
					if (isDirectory)
						fileType = "folder";

					auto name = getSourceWithSuffix(file, target);
					auto key = getHashWithLabel(name);
					node[key]["isa"] = section;
					if (isDirectory)
					{
						node[key]["lastKnownFileType"] = fileType;
					}
					else
					{
						node[key]["explicitFileType"] = fileType;
						node[key]["includeInIndex"] = 0;
						node[key]["name"] = String::getPathFilename(file);
					}

					node[key]["path"] = file;
					node[key]["sourceTree"] = isDirectory ? "<group>" : "SOURCE_ROOT";
				}

				for (auto& file : pbxGroup.headers)
				{
					auto filename = String::getPathFilename(file);
					auto key = getHashWithLabel(file);
					node[key]["isa"] = section;
					node[key]["explicitFileType"] = getXcodeFileTypeFromFile(filename);
					node[key]["includeInIndex"] = 0;
					node[key]["name"] = String::getPathFilename(file);
					node[key]["path"] = file;
					node[key]["sourceTree"] = "<group>";
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

	// PBXCopyFilesBuildPhase
	{
		const std::string section{ "PBXCopyFilesBuildPhase" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (const auto& [target, pbxGroup] : groups)
		{
			if (pbxGroup.kind == TargetGroupKind::AppBundle)
			{
				{
					auto key = getSectionKeyForTarget("CopyFiles", target);
					node[key]["isa"] = section;
					node[key]["buildActionMask"] = kBuildActionMask;
					node[key]["dstPath"] = "";
					node[key]["dstSubfolderSpec"] = DstSubfolderSpec::Executables;
					for (auto& file : pbxGroup.dependencies)
					{
						auto name = getSourceWithSuffix(file, target);
						node[key]["files"].push_back(getHashWithLabel(fmt::format("{} in CopyFiles", name)));
					}
					node[key]["runOnlyForDeploymentPostprocessing"] = 0;
				}

				// libraries that are built outside of the project
				if (!pbxGroup.headers.empty())
				{
					auto key = getSectionKeyForTarget("Embed Libraries", target);
					node[key]["isa"] = section;
					node[key]["buildActionMask"] = kBuildActionMask;
					node[key]["dstPath"] = "";
					node[key]["dstSubfolderSpec"] = DstSubfolderSpec::Frameworks;
					for (auto& file : pbxGroup.headers)
					{
						auto name = getSourceWithSuffix(file, target);
						node[key]["files"].push_back(getHashWithLabel(fmt::format("{} in Embed Libraries", name)));
					}
					node[key]["runOnlyForDeploymentPostprocessing"] = 0;
				}
			}
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
			auto label = pbxGroup.kind == TargetGroupKind::BuildAll ? "Build" : "Sources";
			auto key = getHashWithLabel(fmt::format("{} [{}]", label, target));
			node[key]["isa"] = section;
			node[key]["children"] = Json::array();
			for (auto& child : pbxGroup.children)
			{
				auto name = getSourceWithSuffix(child, target);
				node[key]["children"].push_back(getHashWithLabel(name));
			}
			node[key]["name"] = target;
			node[key]["path"] = pbxGroup.path;
			node[key]["sourceTree"] = group;
			childNodes.emplace_back(std::move(key));
		}

		//
		auto frameworks = getHashWithLabel("Frameworks");
		{
			node[frameworks] = Json::object();
			node[frameworks]["isa"] = section;
			node[frameworks]["children"] = Json::array();
			for (const auto& [target, pbxGroup] : groups)
			{
				if (pbxGroup.kind == TargetGroupKind::AppBundle)
				{
					for (auto& file : pbxGroup.headers)
					{
						node[frameworks]["children"].push_back(getHashWithLabel(file));
					}
				}
			}
			node[frameworks]["children"].push_back(getHashWithLabel(m_allBuildName));
			node[frameworks]["name"] = "Frameworks";
			node[frameworks]["sourceTree"] = group;
		}

		auto products = getHashWithLabel("Products");
		{
			node[products] = Json::object();
			node[products]["isa"] = section;
			node[products]["children"] = Json::array();
			for (const auto& [target, pbxGroup] : groups)
			{
				if (pbxGroup.kind == TargetGroupKind::Source)
				{
					node[products]["children"].push_back(getHashWithLabel(target));
				}
				else if (pbxGroup.kind == TargetGroupKind::AppBundle)
				{
					node[products]["children"].push_back(getHashWithLabel(target));
				}
			}
			node[products]["children"].push_back(getHashWithLabel(m_allBuildName));
			node[products]["name"] = "Products";
			node[products]["sourceTree"] = group;
		}

		//
		node[mainGroup] = Json::object();
		node[mainGroup]["isa"] = section;
		node[mainGroup]["children"] = childNodes;
		node[mainGroup]["children"].push_back(frameworks);
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
			node[key]["target"] = getTargetHashWithLabel(target, pbxGroup.kind == TargetGroupKind::AppBundle);
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
			bool isSource = pbxGroup.kind == TargetGroupKind::Source;
			bool isAppBundle = pbxGroup.kind == TargetGroupKind::AppBundle;
			if (isSource || isAppBundle)
			{
				// TODO: Frameworks
				auto sources = getSectionKeyForTarget("Sources", target);
				auto resources = getSectionKeyForTarget("Resources", target);
				auto key = getTargetHashWithLabel(target, isAppBundle);

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

				if (isSource)
					node[key]["productType"] = getNativeProductType(pbxGroup.targetKind);
				else
					node[key]["productType"] = "com.apple.product-type.application";

				if (isAppBundle)
				{
					auto copyFiles = getSectionKeyForTarget("CopyFiles", target);
					node[key]["buildPhases"].emplace_back(std::move(copyFiles));

					if (!pbxGroup.headers.empty())
					{
						auto embed = getSectionKeyForTarget("Embed Libraries", target);
						node[key]["buildPhases"].emplace_back(std::move(embed));
					}
				}
			}
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
			node[key]["targets"].push_back(getTargetHashWithLabel(target, pbxGroup.kind == TargetGroupKind::AppBundle));
		}
	}

	// PBXResourcesBuildPhase
	{
		const std::string section{ "PBXResourcesBuildPhase" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (const auto& [target, pbxGroup] : groups)
		{
			if (pbxGroup.kind == TargetGroupKind::Source || pbxGroup.kind == TargetGroupKind::AppBundle)
			{
				auto key = getSectionKeyForTarget("Resources", target);
				node[key]["isa"] = section;
				node[key]["buildActionMask"] = kBuildActionMask;
				node[key]["files"] = Json::array();
				for (auto& file : pbxGroup.resources)
				{
					auto name = getSourceWithSuffix(file, target);
					node[key]["files"].push_back(getHashWithLabel(fmt::format("{} in Resources", name)));
				}

				node[key]["runOnlyForDeploymentPostprocessing"] = 0;
			}
		}
	}

	// PBXShellScriptBuildPhase
	{
		const std::string section{ "PBXShellScriptBuildPhase" };
		objects[section] = Json::object();
		auto& node = objects.at(section);

		for (const auto& [target, pbxGroup] : groups)
		{
			if (pbxGroup.kind != TargetGroupKind::Script)
				continue;

			if (!pbxGroup.sources.empty())
			{
				auto makefilePath = fmt::format("{}/scripts/{}.mk", m_exportPath, Hash::uint64(target));
				auto shellScript = fmt::format(R"shell(set -e
if [ -n "$BUILD_FROM_CHALET" ]; then echo "*== script start ==*"; fi
make -f {} --no-builtin-rules --no-builtin-variables --no-print-directory $CONFIGURATION
if [ -n "$BUILD_FROM_CHALET" ]; then echo "*== script end ==*"; fi
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

	// PBXSourcesBuildPhase
	{
		const std::string section{ "PBXSourcesBuildPhase" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (const auto& [target, pbxGroup] : groups)
		{
			bool isSource = pbxGroup.kind == TargetGroupKind::Source;
			if (isSource || pbxGroup.kind == TargetGroupKind::AppBundle)
			{
				auto key = getSectionKeyForTarget("Sources", target);
				node[key]["isa"] = section;
				node[key]["buildActionMask"] = kBuildActionMask;
				node[key]["files"] = Json::array();

				for (const auto& file : pbxGroup.sources)
				{
					auto name = getSourceWithSuffix(file, target);
					node[key]["files"].push_back(getHashWithLabel(fmt::format("{} in Sources", name)));
				}

				node[key]["runOnlyForDeploymentPostprocessing"] = 0;
			}
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
			StringList addedTargets;
			const auto& configName = state->configuration.name();
			for (auto& target : state->targets)
			{
				auto hash = getTargetConfigurationHash(configName, target->name());
				auto key = getHashWithLabel(hash, configName);
				node[key]["isa"] = section;
				if (target->isSources())
				{
					node[key]["buildSettings"] = getBuildSettings(*state, static_cast<const SourceTarget&>(*target));
				}
				else
				{
					node[key]["buildSettings"] = getGenericBuildSettings(*state, *target);
				}
				// node[key]["baseConfigurationReference"] = "";
				node[key]["name"] = configName;

				addedTargets.emplace_back(target->name());
			}

			for (const auto& [target, pbxGroup] : groups)
			{
				if (pbxGroup.kind == TargetGroupKind::Source)
				{
					if (List::contains(addedTargets, target))
						continue;

					auto hash = getTargetConfigurationHash(configName, target);
					auto key = getHashWithLabel(hash, configName);
					node[key]["isa"] = section;
					node[key]["buildSettings"] = getExcludedBuildSettings(*state, target);
					// node[key]["baseConfigurationReference"] = "";
					node[key]["name"] = configName;
				}
			}
		}
		for (auto& state : m_states)
		{
			const auto& configName = state->configuration.name();
			for (auto& target : state->distribution)
			{
				if (target->isDistributionBundle())
				{
					auto name = target->name();
					if (List::contains(sourceTargets, name))
						name += '_';

					auto hash = getTargetConfigurationHash(configName, name, true);
					auto key = getHashWithLabel(hash, configName);
					node[key]["isa"] = section;
					node[key]["buildSettings"] = getAppBundleBuildSettings(*state, static_cast<const BundleTarget&>(*target));
					// node[key]["baseConfigurationReference"] = "";
					node[key]["name"] = configName;
				}
			}
		}
	}

	// XCConfigurationList
	{
		auto project = getProjectName();
		const std::string section{ "XCConfigurationList" };
		objects[section] = Json::object();

		auto& node = objects.at(section);
		{
			StringList configurations;
			for (auto& [configName, _] : configToTargets)
			{
				auto hash = getConfigurationHash(configName);
				configurations.emplace_back(getHashWithLabel(hash, configName));
			}

			auto key = getHashWithLabel(getBuildConfigurationListLabel(project, ListType::Project));
			node[key]["isa"] = section;
			node[key]["buildConfigurations"] = std::move(configurations);
			node[key]["defaultConfigurationIsVisible"] = 0;
			node[key]["defaultConfigurationName"] = firstState.configuration.name();
		}

		for (const auto& [target, pbxGroup] : groups)
		{
			StringList configurations;
			for (auto& [configName, _] : configToTargets)
			{
				auto hash = getTargetConfigurationHash(configName, target, pbxGroup.kind == TargetGroupKind::AppBundle);
				configurations.emplace_back(getHashWithLabel(hash, configName));
			}

			auto type = pbxGroup.kind == TargetGroupKind::Source || pbxGroup.kind == TargetGroupKind::AppBundle ? ListType::NativeProject : ListType::AggregateTarget;
			auto key = getHashWithLabel(getBuildConfigurationListLabel(target, type));
			node[key]["isa"] = section;
			node[key]["buildConfigurations"] = std::move(configurations);
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

	if (Files::pathExists(inFilename))
	{
		auto contentHash = Hash::uint64(contents);
		auto existing = Files::getFileContents(inFilename);
		if (!existing.empty())
		{
			existing.pop_back();
			auto existingHash = Hash::uint64(existing);
			replaceContents = existingHash != contentHash;
		}
	}

	// LOG(contents);

	if (replaceContents && !Files::createFileWithContents(inFilename, contents))
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
Uuid XcodePBXProjGen::getDistTargetHash(const std::string& inTarget) const
{
	return Uuid::v5(fmt::format("{}_DIST_TARGET", inTarget), m_xcodeNamespaceGuid);
}

/*****************************************************************************/
Uuid XcodePBXProjGen::getConfigurationHash(const std::string& inConfig) const
{
	return Uuid::v5(fmt::format("{}_PROJECT", inConfig), m_xcodeNamespaceGuid);
}

/*****************************************************************************/
Uuid XcodePBXProjGen::getTargetConfigurationHash(const std::string& inConfig, const std::string& inTarget, const bool inDist) const
{
	auto suffix = inDist ? "DIST_TARGET" : "TARGET";
	return Uuid::v5(fmt::format("{}-{}_{}", inConfig, inTarget, suffix), m_xcodeNamespaceGuid);
}

/*****************************************************************************/
std::string XcodePBXProjGen::getTargetHashWithLabel(const std::string& inTarget, const bool inDist) const
{
	if (inDist)
		return getHashWithLabel(getDistTargetHash(inTarget), inTarget);
	else
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

	auto& firstState = *m_states.front();

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
	else if (String::equals(firstState.paths.objectiveCppExtension(), ext))
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
	else if (String::equals("app", ext))
		return "wrapper.application";
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
	else if (String::equals(firstState.paths.objectiveCExtensions(), ext))
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
Json XcodePBXProjGen::getProductBuildSettings(const BuildState& inState) const
{
	Json ret;

	auto distDir = Files::getCanonicalPath(inState.inputs.distributionDirectory());
	auto buildDir = Files::getCanonicalPath(inState.paths.outputDirectory());
	auto buildOutputDir = Files::getCanonicalPath(inState.paths.buildOutputDir());

	auto arches = inState.inputs.universalArches();
	if (arches.empty())
		ret["ARCHS"] = inState.info.targetArchitectureString();

	ret["BUILD_DIR"] = buildDir;
	ret["CONFIGURATION_BUILD_DIR"] = buildOutputDir;
	ret["DSTROOT"] = distDir;
	ret["EAGER_LINKING"] = getBoolString(false);
	ret["OBJROOT"] = buildOutputDir;
	ret["PROJECT_RUN_PATH"] = inState.inputs.workingDirectory();
	ret["SDKROOT"] = inState.tools.getApplePlatformSdk(inState.inputs.osTargetName());
	ret["SHARED_PRECOMPS_DIR"] = buildOutputDir;

	// ret["BUILD_ROOT"] = Files::getCanonicalPath(inState.paths.buildOutputDir());
	// ret["SYMROOT"] = buildOutputDir;

	return ret;
}

/*****************************************************************************/
Json XcodePBXProjGen::getBuildSettings(BuildState& inState, const SourceTarget& inTarget) const
{
	const auto& config = inState.configuration;

	CommandAdapterClang clangAdapter(inState, inTarget);

	auto lang = inTarget.language();
	inState.paths.setBuildDirectoriesBasedOnProjectKind(inTarget);

	auto buildOutputDir = Files::getCanonicalPath(inState.paths.buildOutputDir());
	auto objectDirectory = fmt::format("{}/obj.{}", buildOutputDir, inTarget.name());

	Json ret;

	ret["ALWAYS_SEARCH_USER_PATHS"] = getBoolString(false);
	ret["CLANG_ANALYZER_NONNULL"] = getBoolString(true);
	ret["CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION"] = "YES_AGGRESSIVE";
	ret["CLANG_CXX_LANGUAGE_STANDARD"] = clangAdapter.getLanguageStandardCpp();
	ret["CLANG_CXX_LIBRARY"] = clangAdapter.getCxxLibrary();
	ret["CLANG_ENABLE_MODULES"] = getBoolString(inTarget.objectiveCxx());

	if (inTarget.objectiveCxx())
	{
		ret["CLANG_ENABLE_OBJC_ARC"] = getBoolString(false);
		ret["CLANG_ENABLE_OBJC_WEAK"] = getBoolString(true);
	}

	ret["CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING"] = getBoolString(true);
	ret["CLANG_WARN_BOOL_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_COMMA"] = getBoolString(false);
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

	auto& sdk = inState.inputs.osTargetName();
	auto& signingCertificate = inState.tools.signingCertificate();
	if (!sdk.empty())
	{
		ret[fmt::format("CODE_SIGN_IDENTITY[sdk={}*]", sdk)] = !signingCertificate.empty() ? signingCertificate : "-";
		ret["CODE_SIGN_INJECT_BASE_ENTITLEMENTS"] = getBoolString(false);
	}
	else
	{
		ret["CODE_SIGN_IDENTITY"] = !signingCertificate.empty() ? signingCertificate : "-";
		ret["CODE_SIGN_INJECT_BASE_ENTITLEMENTS"] = getBoolString(false);
	}

	ret["CODE_SIGN_STYLE"] = "Manual";

	ret["CONFIGURATION_TEMP_DIR"] = objectDirectory;

	ret["COPY_PHASE_STRIP"] = getBoolString(false);

	if (config.debugSymbols())
	{
		ret["DEBUG_INFORMATION_FORMAT"] = "dwarf-with-dsym";
	}
	if (!sdk.empty())
		ret[fmt::format("DEVELOPMENT_TEAM[sdk={}*]", sdk)] = inState.tools.signingDevelopmentTeam();
	else
		ret["DEVELOPMENT_TEAM"] = inState.tools.signingDevelopmentTeam();

	// ret["ENABLE_NS_ASSERTIONS"] = getBoolString(false);

	// ret["DERIVED_FILE_DIR"] = objectDirectory; // $(CONFIGURATION_TEMP_DIR)/DerivedSources dir

	if (inTarget.objectiveCxx())
	{
		ret["ENABLE_STRICT_OBJC_MSGSEND"] = getBoolString(true);
	}
	ret["ENABLE_TESTABILITY"] = getBoolString(true);

	if (inTarget.isStaticLibrary())
	{
		ret["EXECUTABLE_PREFIX"] = "lib";
		ret["EXECUTABLE_SUFFIX"] = inState.environment->getArchiveExtension();
	}
	else if (inTarget.isSharedLibrary())
	{
		ret["EXECUTABLE_PREFIX"] = "lib";
		ret["EXECUTABLE_SUFFIX"] = inState.environment->getSharedLibraryExtension();
	}

	ret["FRAMEWORK_FLAG_PREFIX"] = "-framework";

	// include dirs
	{
		StringList searchPaths;
		const auto& includeDirs = inTarget.includeDirs();
		const auto& objDir = inState.paths.objDir();
		const auto& externalBuildDir = inState.paths.externalBuildDir();
		for (auto& include : includeDirs)
		{
			if (String::equals(objDir, include))
			{
				searchPaths.emplace_back(objectDirectory);
			}
			else if (String::startsWith(externalBuildDir, include))
			{
				searchPaths.emplace_back(Files::getCanonicalPath(include));
			}
			else
			{
				auto temp = Files::getCanonicalPath(include);
				if (Files::pathExists(temp))
					searchPaths.emplace_back(std::move(temp));
				else
					searchPaths.emplace_back(include);
			}
		}

		searchPaths.emplace_back("$(inherited)");
		ret["HEADER_SEARCH_PATHS"] = std::move(searchPaths);
	}

	ret["LIBRARY_FLAG_PREFIX"] = "-l";

	ret["GENERATE_PROFILING_CODE"] = getBoolString(config.enableProfiling());

	if (inTarget.usesPrecompiledHeader())
	{
		ret["GCC_PREFIX_HEADER"] = Files::getCanonicalPath(inTarget.precompiledHeader());
		ret["GCC_PRECOMPILE_PREFIX_HEADER"] = getBoolString(true);
	}

	ret["GCC_C_LANGUAGE_STANDARD"] = clangAdapter.getLanguageStandardC();

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
		// const auto& workspaceSearchPaths = inState.workspace.searchPaths();
		const auto& objDir = inState.paths.objDir();
		const auto& externalDir = inState.inputs.externalDirectory();
		const auto& externalBuildDir = inState.paths.externalBuildDir();
		// const auto& appleFrameworkPaths = inTarget.appleFrameworkPaths();

		for (auto& libDir : libDirs)
		{
			if (String::equals(objDir, libDir))
			{
				searchPaths.emplace_back(objectDirectory);
			}
			else if (String::startsWith(externalBuildDir, libDir) || String::startsWith(externalDir, libDir))
			{
				auto temp = Files::getCanonicalPath(libDir);
				// runPaths.emplace_back(temp);
				searchPaths.emplace_back(temp);
			}
			else
			{
				auto temp = Files::getCanonicalPath(libDir);
				if (Files::pathExists(temp))
				{
					// runPaths.emplace_back(temp);
					searchPaths.emplace_back(temp);
				}
				else
				{
					// runPaths.emplace_back(libDir);
					searchPaths.emplace_back(libDir);
				}
			}
		}
		// for (auto& path : appleFrameworkPaths)
		// {
		// 	if (String::startsWith(externalBuildDir, path) || String::startsWith(externalDir, path))
		// 	{
		// 		auto temp = Files::getCanonicalPath(path);
		// 		runPaths.emplace_back(temp);
		// 	}
		// 	else
		// 	{
		// 		runPaths.emplace_back(path);
		// 	}
		// }
		// for (auto& path : workspaceSearchPaths)
		// {
		// 	runPaths.emplace_back(path);
		// }

		if (inTarget.isExecutable())
		{
			if (List::contains(m_appBuildTargets, inTarget.name()))
			{
				runPaths.emplace_back("@executable_path/../MacOS");
				runPaths.emplace_back("@executable_path/../Frameworks");
				runPaths.emplace_back("@executable_path/../Resources");
			}
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

	// ret["PRODUCT_BUNDLE_IDENTIFIER"] = "com.developer.application";

	ret["PRODUCT_NAME"] = "$(TARGET_NAME)";

	// if (!sdk.empty())
	// 	ret[fmt::format("PROVISIONING_PROFILE_SPECIFIER[sdk={}*]", sdk)] = "";
	// else
	// 	ret["PROVISIONING_PROFILE_SPECIFIER"] = "";

	ret["TARGET_TEMP_DIR"] = objectDirectory;
	ret["USE_HEADERMAP"] = getBoolString(false);

	// ret["BUILD_ROOT"] = Files::getCanonicalPath(inState.paths.buildOutputDir());
	// ret["SWIFT_OBJC_BRIDGING_HEADER"] = "";

	return ret;
}

/*****************************************************************************/
Json XcodePBXProjGen::getGenericBuildSettings(BuildState& inState, const IBuildTarget& inTarget) const
{
	auto buildOutputDir = Files::getCanonicalPath(inState.paths.buildOutputDir());
	// auto objectDirectory = fmt::format("{}/obj.{}", buildOutputDir, inTarget.name());

	UNUSED(inTarget);
	Json ret;

	ret["ALWAYS_SEARCH_USER_PATHS"] = getBoolString(false);

	ret["CONFIGURATION_TEMP_DIR"] = buildOutputDir;
	ret["OBJECT_FILE_DIR"] = buildOutputDir;
	ret["TARGET_TEMP_DIR"] = buildOutputDir;

	// ret["SWIFT_OBJC_BRIDGING_HEADER"] = "";

	return ret;
}

/*****************************************************************************/
Json XcodePBXProjGen::getExcludedBuildSettings(BuildState& inState, const std::string& inTargetName) const
{
	auto buildOutputDir = Files::getCanonicalPath(inState.paths.buildOutputDir());
	auto objectDirectory = fmt::format("{}/obj.{}", buildOutputDir, inTargetName);

	Json ret;

	ret["ALWAYS_SEARCH_USER_PATHS"] = getBoolString(false);

	ret["CONFIGURATION_TEMP_DIR"] = objectDirectory;
	ret["EXCLUDED_ARCHS"] = "$(ARCHS)"; // Excludes the target on this arch (and configuration)
	ret["OBJECT_FILE_DIR"] = objectDirectory;
	ret["TARGET_TEMP_DIR"] = objectDirectory;

	return ret;
}

/*****************************************************************************/
Json XcodePBXProjGen::getAppBundleBuildSettings(BuildState& inState, const BundleTarget& inTarget) const
{
	auto& targetName = inTarget.name();

	BinaryDependencyMap dependencyMap(inState);
	AppBundlerMacOS bundler(inState, inTarget, dependencyMap);

	auto objectDirectory = Files::getCanonicalPath(inState.paths.bundleObjDir(inTarget.name()));
	auto bundleDirectory = fmt::format("{}/{}", m_exportPath, targetName);
	auto infoPlist = fmt::format("{}/Info.plist", bundleDirectory);
	auto entitlementsPlist = fmt::format("{}/App.entitlements", bundleDirectory);
	auto assetsPath = fmt::format("{}/Assets.xcassets", bundleDirectory);

	if (!Files::pathExists(bundleDirectory))
		Files::makeDirectory(bundleDirectory);

#if defined(CHALET_MACOS)
	auto& macosBundleIcon = inTarget.macosBundleIcon();
#endif

	if (!m_generatedBundleFiles[targetName])
	{
		m_infoPlistJson.clear();

		bundler.setOutputDirectory(objectDirectory);
		bundler.initializeState();

#if defined(CHALET_MACOS)
		if (String::endsWith(".iconset", macosBundleIcon))
			bundler.createIcnsFromIconSet(bundleDirectory);
		else
			bundler.createAssetsXcassets(assetsPath);

		if (inTarget.willHaveMacosInfoPlist())
			bundler.createInfoPropertyListAndReplaceVariables(infoPlist, &m_infoPlistJson);

		if (inTarget.willHaveMacosEntitlementsPlist())
			bundler.createEntitlementsPropertyList(entitlementsPlist);
#endif

		m_generatedBundleFiles[targetName] = true;
	}

	std::string bundleId;
	if (m_infoPlistJson.contains("CFBundleIdentifier") && m_infoPlistJson["CFBundleIdentifier"].is_string())
	{
		bundleId = m_infoPlistJson["CFBundleIdentifier"].get<std::string>();
	}
	else
	{
		bundleId = "com.developer.application";
	}

	//

	Json ret;

	ret["ALWAYS_SEARCH_USER_PATHS"] = getBoolString(false);

#if defined(CHALET_MACOS)
	if (!String::endsWith({ ".icns", "iconset" }, macosBundleIcon))
	{
		ret["ASSETCATALOG_COMPILER_APPICON_NAME"] = bundler.getResolvedIconName();
		// ret["ASSETCATALOG_COMPILER_GLOBAL_ACCENT_COLOR_NAME"] = "AccentColor";
	}
#endif

	ret["BUILD_OUTPUT_DIR"] = inState.paths.buildOutputDir();

	// auto& signingIdentity = inState.tools.signingIdentity();
	std::string signingIdentity;
#if defined(CHALET_MACOS)
	if (inTarget.willHaveMacosEntitlementsPlist())
		ret["CODE_SIGN_ENTITLEMENTS"] = entitlementsPlist;
#endif

	ret["CODE_SIGN_ALLOW_ENTITLEMENTS_MODIFICATION"] = getBoolString(true);

	auto& sdk = inState.inputs.osTargetName();
	auto& signingCertificate = inState.tools.signingCertificate();
	if (!sdk.empty())
	{
		ret[fmt::format("CODE_SIGN_IDENTITY[sdk={}*]", sdk)] = !signingCertificate.empty() ? signingCertificate : "-";
		ret["CODE_SIGN_INJECT_BASE_ENTITLEMENTS"] = getBoolString(false);
	}
	else
	{
		ret["CODE_SIGN_IDENTITY"] = !signingCertificate.empty() ? signingCertificate : "-";
		ret["CODE_SIGN_INJECT_BASE_ENTITLEMENTS"] = getBoolString(false);
	}

	ret["CODE_SIGN_STYLE"] = "Manual";

	ret["CONFIGURATION_TEMP_DIR"] = objectDirectory;
	ret["COPY_PHASE_STRIP"] = getBoolString(false);
	ret["CURRENT_PROJECT_VERSION"] = inState.workspace.metadata().versionString();

	if (!sdk.empty())
		ret[fmt::format("DEVELOPMENT_TEAM[sdk={}*]", sdk)] = inState.tools.signingDevelopmentTeam();
	else
		ret["DEVELOPMENT_TEAM"] = inState.tools.signingDevelopmentTeam();

	ret["EXECUTABLE_NAME"] = bundler.mainExecutable();

	// always set
	ret["INFOPLIST_FILE"] = infoPlist;

	ret["MACOSX_DEPLOYMENT_TARGET"] = inState.inputs.osTargetVersion();
	ret["MARKETING_VERSION"] = inState.workspace.metadata().versionString();

	ret["OBJECT_FILE_DIR"] = objectDirectory;

	ret["PRODUCT_BUNDLE_IDENTIFIER"] = bundleId;
	ret["PRODUCT_NAME"] = "$(TARGET_NAME)";

	if (!sdk.empty())
		ret[fmt::format("PROVISIONING_PROFILE_SPECIFIER[sdk={}*]", sdk)] = "";
	else
		ret["PROVISIONING_PROFILE_SPECIFIER"] = "";

	ret["TARGET_TEMP_DIR"] = objectDirectory;
	ret["USE_HEADERMAP"] = getBoolString(false);

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

	// rpath / executable_path
	if (inTarget.isExecutable())
	{
		ret.emplace_back(fmt::format("-Wl,-install_name,@rpath/{}", String::getPathBaseName(inTarget.outputFile())));
		ret.emplace_back("-Wl,-rpath,@executable_path/.");
	}
	else if (inTarget.isSharedLibrary())
	{
		ret.emplace_back(fmt::format("-Wl,-install_name,@rpath/{}.dylib", String::getPathBaseName(inTarget.outputFile())));
	}

	auto archiveExt = inState.environment->getArchiveExtension();
	auto sharedExt = inState.environment->getSharedLibraryExtension();

	const auto& links = inTarget.links();
	for (auto& link : links)
	{
		if (String::endsWith(sharedExt, link) || String::endsWith(archiveExt, link))
			ret.push_back(Files::getCanonicalPath(link));
		else
			ret.push_back(fmt::format("-l{}", link));
	}
	const auto& staticLinks = inTarget.staticLinks();
	for (auto& link : staticLinks)
	{
		if (String::endsWith(archiveExt, link))
			ret.push_back(Files::getCanonicalPath(link));
		else
			ret.push_back(fmt::format("-l{}", link));
	}

	// Apple Framework Options
	{
		for (auto& path : inTarget.libDirs())
		{
			ret.emplace_back(fmt::format("-F{}", Files::getCanonicalPath(path)));
		}
		for (auto& path : inTarget.appleFrameworkPaths())
		{
			ret.emplace_back(fmt::format("-F{}", Files::getCanonicalPath(path)));
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
