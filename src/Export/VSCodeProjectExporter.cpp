/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSCodeProjectExporter.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Export/VSCode/VSCodeCCppPropertiesGen.hpp"
#include "Export/VSCode/VSCodeLaunchGen.hpp"
#include "Export/VSCode/VSCodeSettingsGen.hpp"
#include "Export/VSCode/VSCodeTasksGen.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
VSCodeProjectExporter::VSCodeProjectExporter(const CommandLineInputs& inInputs) :
	IProjectExporter(inInputs, ExportKind::VisualStudioCodeJSON)
{
}

/*****************************************************************************/
std::string VSCodeProjectExporter::getMainProjectOutput()
{
	if (m_directory.empty())
	{
		if (!useProjectBuildDirectory(".vscode"))
			return std::string();
	}

	return m_directory;
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
	auto output = getMainProjectOutput();
	if (output.empty())
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

		auto clangFormat = fmt::format("{}/.clang-format", state->inputs.workingDirectory());
		if (Commands::pathExists(clangFormat))
		{
			VSCodeSettingsGen settingsJson(outState);
			if (!settingsJson.saveToFile(fmt::format("{}/settings.json", m_directory)))
			{
				Diagnostic::error("There was a problem saving the settings.json file.");
				return false;
			}
		}
	}

	const auto& cwd = workingDirectory();
	auto vscodeDirectory = fmt::format("{}/.vscode", cwd);
	if (!Commands::pathExists(vscodeDirectory) && Commands::pathExists(m_directory))
	{
		if (!Commands::copySilent(m_directory, cwd))
		{
			Diagnostic::error("There was a problem copying the .vscode directory to the workspace.");
			return false;
		}
	}
	else
	{
		auto directory = m_directory;
		String::replaceAll(directory, fmt::format("{}/", cwd), "");
		Diagnostic::warn("The .vscode directory already exists in the workspace root. Copy the files from the following directory to update them: {}", directory);
	}

	return true;
}
}
