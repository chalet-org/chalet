/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSCodeProjectExporter.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Export/VSCode/VSCodeCCppPropertiesGen.hpp"
#include "Export/VSCode/VSCodeLaunchGen.hpp"
#include "Export/VSCode/VSCodeTasksGen.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "Terminal/Commands.hpp"

namespace chalet
{
/*****************************************************************************/
VSCodeProjectExporter::VSCodeProjectExporter(const CommandLineInputs& inInputs) :
	IProjectExporter(inInputs, ExportKind::VisualStudioCodeJSON)
{
}

/*****************************************************************************/
std::string VSCodeProjectExporter::getProjectTypeName() const
{
	return std::string("Visual Studio Code");
}

/*****************************************************************************/
bool VSCodeProjectExporter::validate(const BuildState& inState)
{
	UNUSED(inState);

	return true;
}

/*****************************************************************************/
bool VSCodeProjectExporter::generateProjectFiles()
{
	if (!useProjectBuildDirectory(".vscode"))
		return false;

	const BuildState* state = getAnyBuildStateButPreferDebug();
	chalet_assert(state != nullptr, "");
	if (state != nullptr)
	{
		const auto& outState = *state;
		VSCodeCCppPropertiesGen cCppProperties(outState);
		if (!cCppProperties.saveToFile(fmt::format("{}/c_cpp_properties.json", m_directory)))
		{
			Diagnostic::error("There was a problem saving the c_cpp_properties.json file.");
			return false;
		}

		if (state->configuration.debugSymbols())
		{
			const IBuildTarget* runnableTarget = getRunnableTarget(*state);
			if (runnableTarget != nullptr)
			{
				VSCodeLaunchGen launchJson(outState, *runnableTarget);
				if (!launchJson.saveToFile(fmt::format("{}/launch.json", m_directory)))
				{
					Diagnostic::error("There was a problem saving the launch.json file.");
					return false;
				}

				VSCodeTasksGen tasksJson(outState);
				if (!tasksJson.saveToFile(fmt::format("{}/tasks.json", m_directory)))
				{
					Diagnostic::error("There was a problem saving the tasks.json file.");
					return false;
				}
			}
		}
	}

	return true;
}
}
