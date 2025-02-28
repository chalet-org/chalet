/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSCode/VSCodeSettingsGen.hpp"

#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
VSCodeSettingsGen::VSCodeSettingsGen(const BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
// Note: This just assumes a .clang-format file is present
//   The check is made in VSCodeProjectExporter - maybe rework?
bool VSCodeSettingsGen::saveToFile(const std::string& inFilename) const
{
	Json jRoot;
	jRoot = Json::object();

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
		jRoot["editor.formatOnSave"] = true;
	}
	if (hasC)
	{
		jRoot["[c]"] = R"json({
			"editor.formatOnSave": true
		})json"_ojson;
	}
	if (hasCpp)
	{
		jRoot["[cpp]"] = R"json({
			"editor.formatOnSave": true
		})json"_ojson;
	}
	if (hasObjectiveC)
	{
		jRoot["[objective-c]"] = R"json({
			"editor.formatOnSave": true
		})json"_ojson;
	}
	if (hasObjectiveCpp)
	{
		jRoot["[objective-cpp]"] = R"json({
			"editor.formatOnSave": true
		})json"_ojson;
	}

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

}
