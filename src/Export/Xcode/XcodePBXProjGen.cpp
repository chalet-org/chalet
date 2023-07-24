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
#include "Utility/Timer.hpp"

namespace chalet
{
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

	std::string testA{ "int.new-project in Resources" };
	std::string testB{ "int.new-project" };
	LOG(Uuid::v5(testA, m_projectGuid).toAppleHash(), fmt::format("/* {} */", testA));
	LOG(Uuid::v5(testB, m_projectGuid).toAppleHash(), fmt::format("/* {} */", testB));

	StringList directories{
		"int.new-project",
		"obj.new-project"
	};
	StringList sources{
		"main.cpp"
	};
	StringList headers{
		"pch.hpp"
	};
	StringList targets{
		"new-project"
	};

	Timer timer;

	Json json;
	json["archiveVersion"] = 1;
	json["classes"] = Json::array();
	json["objectVersion"] = kMinimumObjectVersion;
	json["objects"] = Json::object();
	auto& objects = json.at("objects");

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
			node[key]["sourceTree"] = "<group>";
		}
		for (const auto& source : sources)
		{
			auto key = getHashWithLabel(source);
			node[key]["isa"] = section;
			node[key]["path"] = source;
			node[key]["lastKnownFileType"] = "sourcecode.cpp.cpp";
			node[key]["sourceTree"] = "<group>";
		}
		for (const auto& header : headers)
		{
			auto key = getHashWithLabel(header);
			node[key]["isa"] = section;
			node[key]["path"] = header;
			node[key]["lastKnownFileType"] = "sourcecode.cpp.h";
			node[key]["sourceTree"] = "<group>";
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
		//
	}

	// PBXNativeTarget
	{
		const std::string section{ "PBXNativeTarget" };
		objects[section] = Json::object();
		auto& node = objects.at(section);

		// literally the executable(s)
		for (const auto& target : targets)
		{
			auto key = getHashWithLabel(m_projectUUID, target);
			node[key]["isa"] = section;
			node[key]["buildConfigurationList"] = getHashWithLabel(getBuildConfigurationListLabel(target));
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
		node[key]["buildConfigurationList"] = getHashWithLabel(getBuildConfigurationListLabel(name));
		node[key]["compatibilityVersion"] = "Xcode 11.0";
		node[key]["developmentRegion"] = region;
		node[key]["hasScannedForEncodings"] = 0;
		node[key]["knownRegions"] = Json::array();
		node[key]["knownRegions"].push_back("Base");
		node[key]["knownRegions"].push_back(region);
		node[key]["mainGroup"] = 0; // TODO: apple hash pointing to a PBXGroup
		node[key]["projectDirPath"] = "";
		node[key]["projectRoot"] = "";
		node[key]["targets"] = Json::array();
		for (const auto& target : targets)
		{
			auto hash = getHashWithLabel(target);
			node[key]["targets"].push_back(hash);
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
			node[key]["buildSettings"] = getBuildSettings(*state);
			// node[key]["buildSettings"] = Json::object();
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
			configurations.emplace_back(getHashWithLabel(state->configuration.name()));
		}

		auto& node = objects.at(section);
		{
			auto key = getBuildConfigurationListLabel(project, false);
			node[key]["isa"] = section;
			node[key]["buildConfigurations"] = configurations;
			node[key]["defaultConfigurationIsVisible"] = 0;
			node[key]["defaultConfigurationName"] = firstState.configuration.name();
		}

		for (const auto& target : targets)
		{
			auto key = getBuildConfigurationListLabel(target);
			node[key]["isa"] = section;
			node[key]["buildConfigurations"] = configurations;
			node[key]["defaultConfigurationIsVisible"] = 0;
			node[key]["defaultConfigurationName"] = firstState.configuration.name();
		}
	}

	json["rootObject"] = getHashedJsonValue(m_projectUUID, "Project object");

	// LOG(json.dump(2, ' '));

	LOG("Generated in:", timer.asString());

	auto contents = generateFromJson(json);
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
	Timer timer;

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

	LOG(contents);

	LOG("pbxproj generated in:", timer.asString());

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
			if (value.is_string())
			{
				ret += std::string(indent + 1, '\t');
				ret += value.get<std::string>();
				ret += ",\n";
			}
		}

		// ret.pop_back();
		// ret.pop_back();
		// ret += '\n';
		ret += std::string(indent, '\t');
		ret += ')';
	}
	else if (inJson.is_string())
	{
		auto str = inJson.get<std::string>();
		if (str.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_.") == std::string::npos)
		{
			ret += str;
		}
		else
		{
			ret += '"';
			ret += str;
			ret += '"';
		}
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
}
