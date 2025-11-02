/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSCode/VSCodeSettingsGen.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Process/Process.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/DefinesGithub.hpp"
#include "System/DefinesVersion.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
VSCodeSettingsGen::VSCodeSettingsGen(const BuildState& inState, const VSCodeExtensionAwarenessAdapter& inExtensionAdapter) :
	m_state(inState),
	m_extensionAdapter(inExtensionAdapter)
{}

/*****************************************************************************/
bool VSCodeSettingsGen::saveToFile(const std::string& inFilename)
{
	bool chaletExtensionInstalled = m_extensionAdapter.chaletExtensionInstalled();

	auto clangFormat = fmt::format("{}/.clang-format", m_state.inputs.workingDirectory());
	bool clangFormatPresent = Files::pathExists(clangFormat);

	if (!clangFormatPresent && chaletExtensionInstalled)
		return true;

	Json jRoot;
	jRoot = Json::object();

	if (clangFormatPresent)
	{
		setFormatOnSave(jRoot);
	}

	// Use the fallback settings fetching the remote schema
	if (!chaletExtensionInstalled)
	{
		setFallbackSchemaSettings(jRoot);
	}

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
std::string VSCodeSettingsGen::getRemoteSchemaPath(const std::string& inFile) const
{
	return fmt::format("{}/refs/tags/v{}/schema/{}", CHALET_GITHUB_RAW_ROOT, CHALET_VERSION, inFile);
}

/*****************************************************************************/
void VSCodeSettingsGen::setFormatOnSave(Json& outJson) const
{
	bool hasC = false;
	bool hasCpp = false;
	bool hasObjectiveC = false;
	bool hasObjectiveCpp = false;
	bool hasGeneric = false;

	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			auto lang = project.language();
			switch (lang)
			{
				case CodeLanguage::C: hasC = true; break;
				case CodeLanguage::CPlusPlus: hasCpp = true; break;
				case CodeLanguage::ObjectiveC: hasObjectiveC = true; break;
				case CodeLanguage::ObjectiveCPlusPlus: hasObjectiveCpp = true; break;
				default: break;
			}
		}
		else if (target->isSubChalet() || target->isCMake() || target->isMeson())
		{
			// We'll leave it up to the user to configure further
			hasGeneric = true;
		}
	}

	if (hasGeneric)
	{
		outJson["editor.formatOnSave"] = true;
	}
	if (hasC)
	{
		outJson["[c]"] = R"json({
			"editor.formatOnSave": true
		})json"_ojson;
	}
	if (hasCpp)
	{
		outJson["[cpp]"] = R"json({
			"editor.formatOnSave": true
		})json"_ojson;
	}
	if (hasObjectiveC)
	{
		outJson["[objective-c]"] = R"json({
			"editor.formatOnSave": true
		})json"_ojson;
	}
	if (hasObjectiveCpp)
	{
		outJson["[objective-cpp]"] = R"json({
			"editor.formatOnSave": true
		})json"_ojson;
	}
}

/*****************************************************************************/
void VSCodeSettingsGen::setFallbackSchemaSettings(Json& outJson) const
{
	auto chaletJsonSchema = getRemoteSchemaPath("chalet.schema.json");
	auto chaletSettingsJsonSchema = getRemoteSchemaPath("chalet-settings.schema.json");

	auto& jSchemas = outJson["json.schemas"] = Json::array();
	{
		auto jSettingsFile = Json::object();
		auto& jFileMatch = jSettingsFile["fileMatch"] = Json::array();
		jFileMatch.push_back(".chaletrc");
		jSettingsFile["url"] = chaletSettingsJsonSchema;
		jSchemas.emplace_back(std::move(jSettingsFile));
	}
	{
		auto jInputFile = Json::object();
		auto& jFileMatch = jInputFile["fileMatch"] = Json::array();
		jFileMatch.push_back("chalet.json");
		jInputFile["url"] = chaletJsonSchema;
		jSchemas.emplace_back(std::move(jInputFile));
	}

	auto& yamlSchemas = outJson["yaml.schemas"] = Json::object();
	auto& yamlSchemasArray = yamlSchemas[chaletJsonSchema] = Json::array();
	yamlSchemasArray.emplace_back("chalet.yaml");
}

}
