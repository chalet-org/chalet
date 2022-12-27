/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Xcode/XcodeProjectSpecGen.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
XcodeProjectSpecGen::XcodeProjectSpecGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inExportDir) :
	m_states(inStates),
	m_exportDir(inExportDir)
{
	UNUSED(m_states, m_exportDir);
}

/*****************************************************************************/
bool XcodeProjectSpecGen::saveToFile(const std::string& inFilename)
{
	// LOG(inFilename);

	const auto& firstState = *m_states.front();
	for (const auto& target : firstState.targets)
	{
		if (target->isSources())
		{
			const auto& project = static_cast<const SourceTarget&>(*target);
			StringList fileCache;

			auto intermediateDir = firstState.paths.intermediateDir(project);

			if (!Commands::makeDirectory(intermediateDir))
			{
				Diagnostic::error("Error creating paths for project: {}", project.name());
				return false;
			}
		}
	}

	JsonFile jsonFile(inFilename);

	jsonFile.json = Json::object();
	auto& root = jsonFile.json;
	root["name"] = "project";

	root["configs"] = Json::object();
	root["settings"] = Json::object();
	// root["configFiles"] = Json::object();
	{
		auto& configs = root.at("configs");
		// auto& configFiles = root.at("configFiles");
		for (auto& state : m_states)
		{
			const auto& config = state->configuration;
			const auto& name = config.name();

			// TODO: Need these defaults
			std::string preset;
			if (config.debugSymbols())
				preset = "debug";
			else
				preset = "release";

			// configs[name] = "none";
			configs[name] = std::move(preset);
			// configFiles[name] = fmt::format("{}.xcconfig", name);
		}
	}

	root["options"] = Json::object();
	{
		auto& options = root.at("options");
		options["minimumXcodeGenVersion"] = "2.18.0";
		options["developmentLanguage"] = "en";
		options["bundleIdPrefix"] = "com.myapp"; // TODO
	}

	root["targets"] = Json::object();
	{
		auto& targets = root.at("targets");
		StringList includeExcludes{
			"/usr/include",
			"/usr/local/include"
			"/usr/include/",
			"/usr/local/include/"
		};

		for (const auto& target : firstState.targets)
		{
			if (target->isSources())
			{
				const auto& project = static_cast<const SourceTarget&>(*target);
				targets["new-project"] = Json::object();
				auto& tgtJson = targets.at("new-project");

				{
					std::string type;
					if (project.isStaticLibrary())
						type = "library.static";
					else if (project.isSharedLibrary())
						type = "library.dynamic";
					else
						type = "tool";

					tgtJson["type"] = std::move(type);
				}

				tgtJson["platform"] = "macOS";

				// tgtJson["buildRules"] = Json::array();
				// auto& buildRules = tgtJson.at("buildRules");

				// buildRules.emplace_back(Json::object());
				// buildRules[0]["filePattern"] = "*.cpp";
				// buildRules[0]["outputFilesCompilerFlags"] = "-std=c++17 -Wall -Wextra -Werror -Wpedantic -Wunused -Wcast-align -Wdouble-promotion -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wnon-virtual-dtor -Wredundant-decls -Wodr";

				tgtJson["sources"] = Json::array();
				auto& sources = tgtJson.at("sources");
				for (auto& include : project.includeDirs())
				{
					if (String::equals(includeExcludes, include))
						continue;

					Json dir = Json::object();
					dir["path"] = include;
					sources.emplace_back(std::move(dir));
				}

				tgtJson["settings"] = Json::object();
				auto& globalSettings = tgtJson.at("settings");
				globalSettings["configs"] = Json::object();
				auto& settingsConfigs = globalSettings.at("configs");
				for (auto& state : m_states)
				{
					auto settings = getConfigSettings(*state, target->name());
					if (!settings.empty())
					{
						const auto& name = state->configuration.name();
						settingsConfigs[name] = Json::object();
						for (const auto& [key, value] : settings)
						{
							settingsConfigs[name][key] = value;
						}
					}
				}
			}
		}

		// jsonFile.dumpToTerminal();
		jsonFile.setDirty(true);

		if (!jsonFile.save())
		{
			Diagnostic::error("The xcodegen spec file failed to save.");
			return false;
		}

		return true;
	}
}

/*****************************************************************************/
Dictionary<std::string> XcodeProjectSpecGen::getConfigSettings(const BuildState& inState, const std::string& inTarget)
{
	Dictionary<std::string> ret;

	const SourceTarget* project = getProjectFromStateContext(inState, inTarget);
	if (project != nullptr)
	{
		const auto& config = inState.configuration.name();
		auto outputsName = fmt::format("{}_{}", project->name(), config);

		StringList fileCache;
		m_outputs.emplace(outputsName, inState.paths.getOutputs(*project, fileCache));

		const auto& cwd = inState.inputs.workingDirectory();
		// Release
		{
			ret["BUILD_DIR"] = fmt::format("{}/{}", cwd, inState.paths.outputDirectory());
			ret["CONFIGURATION_BUILD_DIR"] = fmt::format("{}/{}", cwd, inState.paths.buildOutputDir());
			ret["CONFIGURATION_TEMP_DIR"] = fmt::format("{}/{}", cwd, inState.paths.objDir());
			ret["OBJECT_FILE_DIR"] = ret.at("CONFIGURATION_TEMP_DIR");
			// ret["BUILD_ROOT"] = fmt::format("{}/{}", cwd, inState.paths.buildOutputDir());
			ret["ALWAYS_SEARCH_USER_PATHS"] = "NO";
			ret["CLANG_ANALYZER_NONNULL"] = "YES";
			ret["CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION"] = "YES_AGGRESSIVE";
			ret["CLANG_CXX_LANGUAGE_STANDARD"] = "c++17";
			ret["CLANG_CXX_LIBRARY"] = "libstdc++";
			ret["CLANG_ENABLE_MODULES"] = "YES";
			ret["CLANG_ENABLE_OBJC_ARC"] = "YES";
			ret["CLANG_ENABLE_OBJC_WEAK"] = "YES";
			ret["CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING"] = "YES";
			ret["CLANG_WARN_BOOL_CONVERSION"] = "YES";
			ret["CLANG_WARN_COMMA"] = "YES";
			ret["CLANG_WARN_CONSTANT_CONVERSION"] = "YES";
			ret["CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS"] = "YES";
			ret["CLANG_WARN_DIRECT_OBJC_ISA_USAGE"] = "YES_ERROR";
			ret["CLANG_WARN_DOCUMENTATION_COMMENTS"] = "YES";
			ret["CLANG_WARN_EMPTY_BODY"] = "YES";
			ret["CLANG_WARN_ENUM_CONVERSION"] = "YES";
			ret["CLANG_WARN_INFINITE_RECURSION"] = "YES";
			ret["CLANG_WARN_INT_CONVERSION"] = "YES";
			ret["CLANG_WARN_NON_LITERAL_NULL_CONVERSION"] = "YES";
			ret["CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF"] = "YES";
			ret["CLANG_WARN_OBJC_LITERAL_CONVERSION"] = "YES";
			ret["CLANG_WARN_OBJC_ROOT_CLASS"] = "YES_ERROR";
			ret["CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER"] = "YES";
			ret["CLANG_WARN_RANGE_LOOP_ANALYSIS"] = "YES";
			ret["CLANG_WARN_STRICT_PROTOTYPES"] = "YES";
			ret["CLANG_WARN_SUSPICIOUS_MOVE"] = "YES";
			ret["CLANG_WARN_UNGUARDED_AVAILABILITY"] = "YES_AGGRESSIVE";
			ret["CLANG_WARN_UNREACHABLE_CODE"] = "YES";
			ret["CLANG_WARN__DUPLICATE_METHOD_MATCH"] = "YES";
			ret["COPY_PHASE_STRIP"] = "NO";
			ret["DEBUG_INFORMATION_FORMAT"] = "dwarf-with-dsym";
			ret["ENABLE_NS_ASSERTIONS"] = "NO";
			ret["ENABLE_STRICT_OBJC_MSGSEND"] = "YES";
			ret["GCC_C_LANGUAGE_STANDARD"] = "gnu11";
			ret["GCC_NO_COMMON_BLOCKS"] = "YES";
			ret["GCC_WARN_64_TO_32_BIT_CONVERSION"] = "YES";
			ret["GCC_WARN_ABOUT_RETURN_TYPE"] = "YES_ERROR";
			ret["GCC_WARN_UNDECLARED_SELECTOR"] = "YES";
			ret["GCC_WARN_UNINITIALIZED_AUTOS"] = "YES_AGGRESSIVE";
			ret["GCC_WARN_UNUSED_FUNCTION"] = "YES";
			ret["GCC_WARN_UNUSED_VARIABLE"] = "YES";
			ret["MACOSX_DEPLOYMENT_TARGET"] = "11.1";
			ret["MTL_ENABLE_DEBUG_INFO"] = "NO";
			ret["MTL_FAST_MATH"] = "YES";
			ret["SDKROOT"] = "macosx";
			// ret["SWIFT_OBJC_BRIDGING_HEADER"] = "";
		}
	}

	return ret;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
// Utils
/*****************************************************************************/
const SourceTarget* XcodeProjectSpecGen::getProjectFromStateContext(const BuildState& inState, const std::string& inName) const
{
	const SourceTarget* ret = nullptr;
	for (auto& target : inState.targets)
	{
		if (target->isSources() && String::equals(target->name(), inName))
		{
			ret = static_cast<const SourceTarget*>(target.get());
		}
	}

	return ret;
}

/*
	Example XcodeGen spec file:

{
	"name": "Chalet",
	"configs": {
		"Debug": "debug",
		"Release": "release"
	},
	"options": {
		"minimumXcodeGenVersion": "2.18.0",
		"developmentLanguage": "en",
		"bundleIdPrefix": "com.myapp"
	},
	"targetTemplates": {
		"AllTargets": {
			"platform": "macOS",
			"buildRules": [
				{
					"filePattern": "*.cpp",
					"outputFilesCompilerFlags": "-std=c++17 -Iexternal -Wall -Wextra -Werror -Wpedantic -Wunused -Wcast-align -Wdouble-promotion -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wnon-virtual-dtor -Wredundant-decls -Wodr"
				}
			]
		}
	},
	"targets": {
		"json-schema-validator": {
			"type": "library.static",
			"templates": [
				"AllTargets"
			],
			"sources": [
				{
					"path": "external/json-schema-validator"
				}
			]
		},
		"chalet": {
			"type": "application",
			"templates": [
				"AllTargets"
			],
			"sources": [
				{
					"path": "src"
				}
			],
			"dependencies": [
				{
					"target": "json-schema-validator"
				}
			]
		},
		"tests": {
			"type": "application",
			"templates": [
				"AllTargets"
			],
			"sources": [
				{
					"path": "src"
				},
				{
					"path": "tests"
				}
			],
			"dependencies": [
				{
					"target": "json-schema-validator"
				}
			]
		}
	}
}
*/
}