/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Zed/ZedSettingsGen.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/DefinesGithub.hpp"
#include "System/DefinesVersion.hpp"
#include "System/Files.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
ZedSettingsGen::ZedSettingsGen(const BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
bool ZedSettingsGen::saveToFile(const std::string& inFilename) const
{
	Json jRoot;
	jRoot = Json::object();
	{
		auto& jFileTypes = jRoot["file_types"] = Json::object();
		auto& jJsonFileTypes = jFileTypes["JSON"] = Json::array();
		jJsonFileTypes.push_back(m_state.inputs.settingsFile());
	}

	// CHALET_VERSION
	{
		auto chaletJsonSchema = getRemoteSchemaPath("chalet.schema.json");
		auto chaletSettingsJsonSchema = getRemoteSchemaPath("chalet-settings.schema.json");

		auto& jLsp = jRoot["lsp"] = Json::object();

		// TODO: Local path schema resolution is pretty broken in Zed
		//   at the moment, it's only confirmed working on macOS
		//   Linux may be working too...
		{
			auto& jJsonLanguageServer = jLsp["json-language-server"] = Json::object();
			auto& jSettings = jJsonLanguageServer["settings"] = Json::object();
			auto& jJson = jSettings["json"] = Json::object();
			auto& jSchemas = jJson["schemas"] = Json::array();

			// const auto& cwd = m_state.inputs.workingDirectory();
			{
				auto jSettingsFile = Json::object();
				auto& jFileMatch = jSettingsFile["fileMatch"] = Json::array();
				jFileMatch.push_back(".chaletrc");
				jSettingsFile["url"] = chaletSettingsJsonSchema;
				// jSettingsFile["url"] = fmt::format("..{}/{}", cwd, chaletSettingsJsonSchema);
				jSchemas.emplace_back(std::move(jSettingsFile));
			}
			{
				auto jInputFile = Json::object();
				auto& jFileMatch = jInputFile["fileMatch"] = Json::array();
				jFileMatch.push_back("chalet.json");
				jInputFile["url"] = chaletJsonSchema;
				// jInputFile["url"] = fmt::format("..{}/{}", cwd, chaletJsonSchema);
				jSchemas.emplace_back(std::move(jInputFile));
			}
		}

		{
			auto& jYamlLanguageServer = jLsp["yaml-language-server"] = Json::object();
			auto& jSettings = jYamlLanguageServer["settings"] = Json::object();
			auto& jYaml = jSettings["yaml"] = Json::object();
			auto& jSchemas = jYaml["schemas"] = Json::object();

			{
				auto& jArray = jSchemas[chaletJsonSchema] = Json::array();
				jArray.emplace_back("chalet.yaml");
			}
		}
	}

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
std::string ZedSettingsGen::getRemoteSchemaPath(const std::string& inFile) const
{
	return fmt::format("{}/refs/tags/v{}/schema/{}", CHALET_GITHUB_RAW_ROOT, CHALET_VERSION, inFile);
}
}
