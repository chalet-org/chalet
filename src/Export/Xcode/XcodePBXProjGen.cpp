/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Xcode/XcodePBXProjGen.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Libraries/Json.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
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

	std::string innerNode;

	const auto& firstState = *m_states.front();
	const auto& workspaceName = firstState.workspace.metadata().name();

	auto projectHashRaw = Uuid::v5(fmt::format("{}_PBXPROJ", workspaceName), m_xcodeNamespaceGuid);
	m_projectGuid = projectHashRaw.str();

	auto projectHash = projectHashRaw.toAppleHash();
	LOG("");
	LOG(projectHash, fmt::format("/* Project object */"));

	std::string testA{ "int.new-project in Resources" };
	std::string testB{ "int.new-project" };
	LOG(Uuid::v5(testA, m_projectGuid).toAppleHash(), fmt::format("/* {} */", testA));
	LOG(Uuid::v5(testB, m_projectGuid).toAppleHash(), fmt::format("/* {} */", testB));

	auto contents = fmt::format(R"pbxproj(// !$*UTF8*$!
{{
	archiveVersion = 1;
	classes = {{
	}};
	objectVersion = {objectVersion};
	objects = {{
{innerNode}
	}};
	rootObject = {projectHash} /* Project object */;
}})pbxproj",
		fmt::arg("objectVersion", kMinimumObjectVersion),
		FMT_ARG(innerNode),
		FMT_ARG(projectHash));

	LOG(contents);

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

	UJson json;
	json["archiveVersion"] = 1;
	json["classes"] = UJson::array();
	json["objectVersion"] = kMinimumObjectVersion;
	json["objects"] = UJson::object();
	auto& objects = json.at("objects");

	// PBXBuildFile
	{
		const std::string section{ "PBXBuildFile" };
		objects[section] = UJson::object();
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
		objects[section] = UJson::object();
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
	{}

	// PBXNativeTarget
	{
		// literally the executable(s)
	}

	// PBXProject
	{
		const std::string section{ "PBXProject" };
		const std::string region{ "en" };
		const std::string name{ "project" };
		objects[section] = UJson::object();
		auto& node = objects.at(section);
		auto key = getHashWithLabel(projectHashRaw, "Project object");
		node[key]["isa"] = section;
		node[key]["attributes"] = UJson::object();
		node[key]["attributes"]["LastUpgradeCheck"] = 1200;
		node[key]["attributes"]["TargetAttributes"] = UJson::object();
		node[key]["buildConfigurationList"] = getHashWithLabel(fmt::format("Build configuration list for PBXProject \"{}\"", name));
		node[key]["compatibilityVersion"] = "Xcode 11.0";
		node[key]["developmentRegion"] = region;
		node[key]["hasScannedForEncodings"] = 0;
		node[key]["knownRegions"] = UJson::array();
		node[key]["knownRegions"].push_back("Base");
		node[key]["knownRegions"].push_back(region);
		node[key]["mainGroup"] = 0; // TODO: apple hash pointing to a PBXGroup
		node[key]["projectDirPath"] = "";
		node[key]["projectRoot"] = "";
		node[key]["targets"] = UJson::array();
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
		objects[section] = UJson::object();
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
		objects[section] = UJson::object();
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
	{}

	// XCConfigurationList
	{
		// List configs for PBXProject and PBXNativeTarget
	}

	json["rootObject"] = getHashedJsonValue(projectHashRaw, "Project object");

	LOG(json.dump(2, ' '));

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
UJson XcodePBXProjGen::getHashedJsonValue(const std::string& inValue) const
{
	auto hash = Uuid::v5(inValue, m_xcodeNamespaceGuid);
	return getHashedJsonValue(hash, inValue);
}

/*****************************************************************************/
UJson XcodePBXProjGen::getHashedJsonValue(const Uuid& inHash, const std::string& inLabel) const
{
	// UJson ret = UJson::object();
	// ret["hash"] = inHash.toAppleHash();
	// ret["label"] = inLabel;
	// return ret;
	return UJson(getHashWithLabel(inHash, inLabel));
}
}
