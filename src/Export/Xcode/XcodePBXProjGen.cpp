/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Xcode/XcodePBXProjGen.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Libraries/Json.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
struct PBXGroup
{
	std::string path;
	StringList children;
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
XcodePBXProjGen::XcodePBXProjGen(const std::vector<Unique<BuildState>>& inStates) :
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

	m_projectUUID = Uuid::v5(fmt::format("{}_PBXPROJ", workspaceName), m_xcodeNamespaceGuid);
	m_projectGuid = m_projectUUID.str();

	StringList directories;
	StringList sources;
	StringList headers;
	StringList targets;
	std::map<std::string, PBXGroup> groups;

	{
		StringList paths;
		for (auto& state : m_states)
		{
			for (auto& target : state->targets)
			{
				if (target->isSources())
				{
					const auto& sourceTarget = static_cast<const SourceTarget&>(*target);
					state->paths.setBuildDirectoriesBasedOnProjectKind(sourceTarget);

					List::addIfDoesNotExist(directories, String::getPathFilename(state->paths.intermediateDir(sourceTarget)));
					List::addIfDoesNotExist(directories, String::getPathFilename(state->paths.objDir()));

					List::addIfDoesNotExist(targets, sourceTarget.outputFile());

					auto& pch = sourceTarget.precompiledHeader();

					for (auto& include : sourceTarget.includeDirs())
					{
						List::addIfDoesNotExist(paths, include);
					}

					for (auto& file : sourceTarget.files())
					{
						List::addIfDoesNotExist(sources, String::getPathFilename(file));
						List::addIfDoesNotExist(paths, file);
					}

					if (!pch.empty())
					{
						List::addIfDoesNotExist(headers, String::getPathFilename(pch));
						List::addIfDoesNotExist(paths, pch);
					}
					auto tmpHeaders = sourceTarget.getHeaderFiles();
					for (auto& file : tmpHeaders)
					{
						List::addIfDoesNotExist(headers, String::getPathFilename(file));
						List::addIfDoesNotExist(paths, file);
					}
				}
			}
		}

		for (auto& pathRaw : paths)
		{
			if (String::startsWith("/usr", pathRaw))
				continue;

			auto child = String::getPathFilename(pathRaw);
			auto name = String::getPathFilename(String::getPathFolder(pathRaw));
			std::string path;
			if (name.empty())
			{
				name = std::move(child);
				path = Commands::getProximatePath(pathRaw, fmt::format("{}/../..", inFilename));
			}
			else
			{
				path = Commands::getProximatePath(String::getPathFolder(pathRaw), fmt::format("{}/../..", inFilename));
			}

			if (groups.find(name) == groups.end())
			{
				groups.emplace(name, PBXGroup{});
				groups[name].path = std::move(path);
			}

			if (!child.empty())
			{
				List::addIfDoesNotExist(groups[name].children, getHashWithLabel(child));
			}
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
	const std::string group{ "<group>" };

	// PBXBuildFile
	{
		const std::string section{ "PBXBuildFile" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (const auto& dir : directories)
		{
			auto key = getHashWithLabel(fmt::format("{} in Resources", dir));
			node[key]["isa"] = section;
			node[key]["fileRef"] = getHashedJsonValue(dir);
		}
		for (const auto& source : sources)
		{
			auto key = getHashWithLabel(fmt::format("{} in Sources", source));
			node[key]["isa"] = section;
			node[key]["fileRef"] = getHashedJsonValue(source);
		}
	}

	// PBXFileReference
	{
		const std::string section{ "PBXFileReference" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (const auto& dir : directories)
		{
			auto key = getHashWithLabel(dir);
			node[key]["isa"] = section;
			node[key]["path"] = dir;
			node[key]["sourceTree"] = group;
		}
		for (const auto& source : sources)
		{
			auto key = getHashWithLabel(source);
			node[key]["isa"] = section;
			node[key]["lastKnownFileType"] = "sourcecode.cpp.cpp";
			node[key]["path"] = source;
			node[key]["sourceTree"] = group;
		}
		for (const auto& header : headers)
		{
			auto key = getHashWithLabel(header);
			node[key]["isa"] = section;
			node[key]["lastKnownFileType"] = "sourcecode.cpp.h";
			node[key]["path"] = header;
			node[key]["sourceTree"] = group;
		}
		for (const auto& target : targets)
		{
			auto key = getHashWithLabel(target);
			node[key]["isa"] = section;
			node[key]["includeInIndex"] = 0;
			node[key]["path"] = target;
			node[key]["sourceTree"] = "BUILT_PRODUCTS_DIR";
		}
	}

	// PBXGroup
	{
		const std::string section{ "PBXGroup" };
		objects[section] = Json::object();
		auto& node = objects.at(section);

		StringList childNodes;
		for (const auto& [name, pbxGroup] : groups)
		{
			auto key = getHashWithLabel(name);
			node[key]["isa"] = section;
			node[key]["children"] = pbxGroup.children;
			node[key]["name"] = name;
			node[key]["path"] = pbxGroup.path;
			node[key]["sourceTree"] = group;
			childNodes.emplace_back(std::move(key));
		}

		node[products] = Json::object();
		node[products]["isa"] = section;
		node[products]["children"] = Json::array();
		for (auto& target : targets)
		{
			node[products]["children"].push_back(getHashWithLabel(target));
		}
		node[products]["name"] = "Products";
		node[products]["sourceTree"] = group;

		node[mainGroup] = Json::object();
		node[mainGroup]["isa"] = section;
		node[mainGroup]["children"] = childNodes;
		node[mainGroup]["children"].emplace_back(products);
		node[mainGroup]["sourceTree"] = group;
	}

	// PBXNativeTarget
	{
		const std::string section{ "PBXNativeTarget" };
		objects[section] = Json::object();
		auto& node = objects.at(section);

		for (const auto& target : targets)
		{
			auto key = getTargetHashWithLabel(target);
			node[key]["isa"] = section;
			node[key]["buildConfigurationList"] = getHashWithLabel(getBuildConfigurationListLabel(target));
			node[key]["buildPhases"] = Json::array();
			node[key]["buildPhases"].push_back(getHashWithLabel("Sources"));
			node[key]["buildPhases"].push_back(getHashWithLabel("Resources"));
			node[key]["buildRules"] = Json::array();
			node[key]["dependencies"] = Json::array();
			node[key]["name"] = target;
			node[key]["productName"] = target;
			node[key]["productReference"] = getHashWithLabel(target);
			node[key]["productType"] = "com.apple.product-type.tool";
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
		node[key]["attributes"]["LastUpgradeCheck"] = 1200;
		node[key]["attributes"]["TargetAttributes"] = Json::object();
		node[key]["buildConfigurationList"] = getHashWithLabel(getBuildConfigurationListLabel(name, false));
		node[key]["compatibilityVersion"] = "Xcode 11.0";
		node[key]["developmentRegion"] = region;
		node[key]["hasScannedForEncodings"] = 0;
		node[key]["knownRegions"] = Json::array();
		node[key]["knownRegions"].push_back("Base");
		node[key]["knownRegions"].push_back(region);
		node[key]["mainGroup"] = mainGroup;
		node[key]["projectDirPath"] = "";
		node[key]["projectRoot"] = "";
		node[key]["targets"] = Json::array();
		for (const auto& target : targets)
		{
			node[key]["targets"].push_back(getTargetHashWithLabel(target));
		}
	}

	// PBXResourcesBuildPhase
	{
		const std::string context{ "Resources" };
		const std::string section{ "PBXResourcesBuildPhase" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		auto key = getHashWithLabel(context);
		node[key]["isa"] = section;
		node[key]["buildActionMask"] = kBuildActionMask;
		node[key]["files"] = Json::array();
		for (const auto& dir : directories)
		{
			node[key]["files"].push_back(getHashWithLabel(fmt::format("{} in {}", dir, context)));
		}
		node[key]["runOnlyForDeploymentPostprocessing"] = 0;
	}

	// PBXSourcesBuildPhase
	{
		const std::string context{ "Sources" };
		const std::string section{ "PBXSourcesBuildPhase" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		auto key = getHashWithLabel(context);
		node[key]["isa"] = section;
		node[key]["buildActionMask"] = kBuildActionMask;
		node[key]["files"] = Json::array();
		for (const auto& source : sources)
		{
			node[key]["files"].push_back(getHashWithLabel(fmt::format("{} in {}", source, context)));
		}
		node[key]["runOnlyForDeploymentPostprocessing"] = 0;
	}

	// XCBuildConfiguration
	{
		const std::string section{ "XCBuildConfiguration" };
		objects[section] = Json::object();
		auto& node = objects.at(section);
		for (auto& state : m_states)
		{
			auto& name = state->configuration.name();
			auto key = getHashWithLabel(name);
			node[key]["isa"] = section;
			// node[key]["buildSettings"] = Json::object();
			node[key]["buildSettings"] = getBuildSettings(*state);
			// node[key]["baseConfigurationReference"] = "";
			node[key]["name"] = name;
		}
		for (auto& state : m_states)
		{
			auto& name = state->configuration.name();
			auto hash = Uuid::v5(fmt::format("{}_PROJECT", name), m_xcodeNamespaceGuid);
			auto key = getHashWithLabel(hash, name);
			node[key]["isa"] = section;
			// node[key]["buildSettings"] = Json::object();
			node[key]["buildSettings"] = getProductBuildSettings(*state);
			// node[key]["baseConfigurationReference"] = "";
			node[key]["name"] = name;
		}
	}

	// XCConfigurationList
	{
		const std::string project{ "project" };
		const std::string section{ "XCConfigurationList" };
		objects[section] = Json::object();

		StringList configurations;
		for (auto& state : m_states)
		{
			auto hash = Uuid::v5(fmt::format("{}_PROJECT", state->configuration.name()), m_xcodeNamespaceGuid);
			configurations.emplace_back(getHashWithLabel(hash, state->configuration.name()));
		}

		auto& node = objects.at(section);
		{
			auto key = getHashWithLabel(getBuildConfigurationListLabel(project, false));
			node[key]["isa"] = section;
			node[key]["buildConfigurations"] = configurations;
			node[key]["defaultConfigurationIsVisible"] = 0;
			node[key]["defaultConfigurationName"] = firstState.configuration.name();
		}

		configurations.clear();
		for (auto& state : m_states)
		{
			configurations.emplace_back(getHashWithLabel(state->configuration.name()));
		}
		for (const auto& target : targets)
		{
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

	// LOG(contents);

	if (!Commands::createFileWithContents(inFilename, contents))
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
Json XcodePBXProjGen::getBuildSettings(const BuildState& inState) const
{
	const auto& cwd = inState.inputs.workingDirectory();

	// TODO: this is currently just based on a Release mode

	Json ret;
	ret["ALWAYS_SEARCH_USER_PATHS"] = getBoolString(false);
	ret["BUILD_DIR"] = fmt::format("{}/{}", cwd, inState.paths.outputDirectory());
	ret["CLANG_ANALYZER_NONNULL"] = getBoolString(true);
	ret["CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION"] = "YES_AGGRESSIVE";
	ret["CLANG_CXX_LANGUAGE_STANDARD"] = "c++17";
	ret["CLANG_CXX_LIBRARY"] = "libc++";
	ret["CLANG_ENABLE_MODULES"] = getBoolString(true);
	ret["CLANG_ENABLE_OBJC_ARC"] = getBoolString(true);
	ret["CLANG_ENABLE_OBJC_WEAK"] = getBoolString(true);
	ret["CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING"] = getBoolString(true);
	ret["CLANG_WARN_BOOL_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_COMMA"] = getBoolString(true);
	ret["CLANG_WARN_CONSTANT_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS"] = getBoolString(true);
	ret["CLANG_WARN_DIRECT_OBJC_ISA_USAGE"] = "YES_ERROR";
	ret["CLANG_WARN_DOCUMENTATION_COMMENTS"] = getBoolString(true);
	ret["CLANG_WARN_EMPTY_BODY"] = getBoolString(true);
	ret["CLANG_WARN_ENUM_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_INFINITE_RECURSION"] = getBoolString(true);
	ret["CLANG_WARN_INT_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_NON_LITERAL_NULL_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF"] = getBoolString(true);
	ret["CLANG_WARN_OBJC_LITERAL_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_OBJC_ROOT_CLASS"] = "YES_ERROR";
	ret["CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER"] = getBoolString(true);
	ret["CLANG_WARN_RANGE_LOOP_ANALYSIS"] = getBoolString(true);
	ret["CLANG_WARN_STRICT_PROTOTYPES"] = getBoolString(true);
	ret["CLANG_WARN_SUSPICIOUS_MOVE"] = getBoolString(true);
	ret["CLANG_WARN_UNGUARDED_AVAILABILITY"] = "YES_AGGRESSIVE";
	ret["CLANG_WARN_UNREACHABLE_CODE"] = getBoolString(true);
	ret["CLANG_WARN__DUPLICATE_METHOD_MATCH"] = getBoolString(true);
	ret["COMBINE_HIDPI_IMAGES"] = getBoolString(true);
	ret["CONFIGURATION_BUILD_DIR"] = fmt::format("{}/{}", cwd, inState.paths.buildOutputDir());
	ret["CONFIGURATION_TEMP_DIR"] = fmt::format("{}/{}", cwd, inState.paths.objDir());
	ret["COPY_PHASE_STRIP"] = getBoolString(false);
	ret["DEBUG_INFORMATION_FORMAT"] = "dwarf-with-dsym";
	ret["ENABLE_NS_ASSERTIONS"] = getBoolString(false);
	ret["ENABLE_STRICT_OBJC_MSGSEND"] = getBoolString(true);
	ret["GCC_C_LANGUAGE_STANDARD"] = "gnu11";
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
	ret["MACOSX_DEPLOYMENT_TARGET"] = inState.inputs.osTargetVersion();
	ret["MTL_ENABLE_DEBUG_INFO"] = getBoolString(inState.configuration.debugSymbols());
	ret["MTL_FAST_MATH"] = getBoolString(false);
	ret["OBJECT_FILE_DIR"] = ret.at("CONFIGURATION_TEMP_DIR");
	ret["PRODUCT_BUNDLE_IDENTIFIER"] = getProductBundleIdentifier(inState.workspace.metadata().name());
	ret["SDKROOT"] = inState.inputs.osTargetName();

	// ret["BUILD_ROOT"] = fmt::format("{}/{}", cwd, inState.paths.buildOutputDir());
	// ret["SWIFT_OBJC_BRIDGING_HEADER"] = "";
	return ret;
}

/*****************************************************************************/
Json XcodePBXProjGen::getProductBuildSettings(const BuildState& inState) const
{
	// TODO: this is currently just based on a Debug mode

	Json ret;
	ret["ALWAYS_SEARCH_USER_PATHS"] = getBoolString(false);
	ret["CLANG_ANALYZER_NONNULL"] = getBoolString(true);
	ret["CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION"] = "YES_AGGRESSIVE";
	ret["CLANG_CXX_LANGUAGE_STANDARD"] = "c++17";
	ret["CLANG_CXX_LIBRARY"] = "libc++";
	ret["CLANG_ENABLE_MODULES"] = getBoolString(true);
	ret["CLANG_ENABLE_OBJC_ARC"] = getBoolString(true);
	ret["CLANG_ENABLE_OBJC_WEAK"] = getBoolString(true);
	ret["CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING"] = getBoolString(true);
	ret["CLANG_WARN_BOOL_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_COMMA"] = getBoolString(true);
	ret["CLANG_WARN_CONSTANT_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS"] = getBoolString(true);
	ret["CLANG_WARN_DIRECT_OBJC_ISA_USAGE"] = "YES_ERROR";
	ret["CLANG_WARN_DOCUMENTATION_COMMENTS"] = getBoolString(true);
	ret["CLANG_WARN_EMPTY_BODY"] = getBoolString(true);
	ret["CLANG_WARN_ENUM_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_INFINITE_RECURSION"] = getBoolString(true);
	ret["CLANG_WARN_INT_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_NON_LITERAL_NULL_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF"] = getBoolString(true);
	ret["CLANG_WARN_OBJC_LITERAL_CONVERSION"] = getBoolString(true);
	ret["CLANG_WARN_OBJC_ROOT_CLASS"] = "YES_ERROR";
	ret["CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER"] = getBoolString(true);
	ret["CLANG_WARN_RANGE_LOOP_ANALYSIS"] = getBoolString(true);
	ret["CLANG_WARN_STRICT_PROTOTYPES"] = getBoolString(true);
	ret["CLANG_WARN_SUSPICIOUS_MOVE"] = getBoolString(true);
	ret["CLANG_WARN_UNGUARDED_AVAILABILITY"] = "YES_AGGRESSIVE";
	ret["CLANG_WARN_UNREACHABLE_CODE"] = getBoolString(true);
	ret["CLANG_WARN__DUPLICATE_METHOD_MATCH"] = getBoolString(true);
	ret["COPY_PHASE_STRIP"] = getBoolString(false);
	ret["DEBUG_INFORMATION_FORMAT"] = "dwarf-with-dsym";
	ret["ENABLE_STRICT_OBJC_MSGSEND"] = getBoolString(true);
	ret["ENABLE_TESTABILITY"] = getBoolString(true);
	ret["GCC_C_LANGUAGE_STANDARD"] = "gnu11";
	ret["GCC_DYNAMIC_NO_PIC"] = getBoolString(false);
	ret["GCC_NO_COMMON_BLOCKS"] = getBoolString(true);
	ret["GCC_OPTIMIZATION_LEVEL"] = 0;
	ret["GCC_PREPROCESSOR_DEFINITIONS"] = {
		"$(inherited)",
		"DEBUG=1",
	};
	ret["GCC_WARN_64_TO_32_BIT_CONVERSION"] = getBoolString(true);
	ret["GCC_WARN_ABOUT_RETURN_TYPE"] = "YES_ERROR";
	ret["GCC_WARN_UNDECLARED_SELECTOR"] = getBoolString(true);
	ret["GCC_WARN_UNINITIALIZED_AUTOS"] = "YES_AGGRESSIVE";
	ret["GCC_WARN_UNUSED_FUNCTION"] = getBoolString(true);
	ret["GCC_WARN_UNUSED_VARIABLE"] = getBoolString(true);
	ret["MTL_ENABLE_DEBUG_INFO"] = "INCLUDE_SOURCE";
	ret["MTL_FAST_MATH"] = getBoolString(false);
	ret["ONLY_ACTIVE_ARCH"] = getBoolString(true);
	ret["PRODUCT_NAME"] = "$(TARGET_NAME)";
	ret["SDKROOT"] = inState.inputs.osTargetName();
	// ret["LD_RUNPATH_SEARCH_PATHS"] = {
	// 	"$(inherited)",
	// 	"@executable_path/../Frameworks",
	// 	"@loader_path/../Frameworks",
	// };

	// Swift?
	// ret["SWIFT_ACTIVE_COMPILATION_CONDITIONS"] = "DEBUG";
	// ret["SWIFT_OPTIMIZATION_LEVEL"] = "-Onone";
	// ret["SWIFT_OBJC_BRIDGING_HEADER"] = "$(SRCROOT)/src/SwiftBridgeHeader.h";
	// ret["SWIFT_VERSION"] = "5.0";

	// Code signing
	// ret["CODE_SIGN_STYLE"] = "Automatic";
	// ret["DEVELOPMENT_TEAM"] = ""; // required!

	ret["PRODUCT_NAME"] = "$(TARGET_NAME)";

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

	if (!str.empty() && (startsWithHash || str.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_./") == std::string::npos))
	{
		return str;
	}
	else
	{
		return fmt::format("\"{}\"", str);
	}
}
}
