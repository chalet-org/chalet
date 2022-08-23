/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Xcode/XcodeProjectSpecGen.hpp"

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
bool XcodeProjectSpecGen::saveToFile(const std::string& inFilename) const
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
	{
		auto& configs = root.at("configs");
		for (auto& state : m_states)
		{
			const auto& config = state->configuration;

			// TODO: we want to supply everything, not use a preset
			std::string preset;
			if (config.debugSymbols())
				preset = "debug";
			else
				preset = "release";

			configs[config.name()] = std::move(preset);
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
