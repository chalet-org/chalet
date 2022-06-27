/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSCodeProjectExporter.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Export/VSCode/CCppPropertiesGen.hpp"
#include "Export/VSCode/VSCodeLaunchGen.hpp"
#include "Export/VSCode/VSCodeTasksGen.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"

namespace chalet
{
/*****************************************************************************/
VSCodeProjectExporter::VSCodeProjectExporter(CentralState& inCentralState) :
	IProjectExporter(inCentralState, ExportKind::VisualStudioCode)
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
	if (!useExportDirectory("vscode"))
		return false;

	const std::string vscodeDir{ ".vscode" };
	if (!Commands::pathExists(vscodeDir))
	{
		if (!Commands::makeDirectory(vscodeDir))
		{
			Diagnostic::error("There was a problem creating the '{}' directory.", vscodeDir);
			return false;
		}
	}

	const BuildState* state = nullptr;
	if (!m_states.empty())
	{
		if (m_debugConfiguration.empty())
			state = m_states.begin()->second.get();
		else
			state = m_states.at(m_debugConfiguration).get();
	}

	if (state != nullptr)
	{
		const auto& outState = *state;
		CCppPropertiesGen cCppProperties(outState, m_cwd);
		if (!cCppProperties.saveToFile(fmt::format("{}/c_cpp_properties.json", vscodeDir)))
		{
			Diagnostic::error("There was a problem saving the c_cpp_properties.json file.");
			return false;
		}

		if (state->configuration.debugSymbols())
		{
			const IBuildTarget* runnableTarget = nullptr;
			for (auto& target : state->targets)
			{
				if (target->isSources())
				{
					const auto& project = static_cast<const SourceTarget&>(*target);
					if (project.isExecutable())
					{
						runnableTarget = target.get();
						break;
					}
				}
				else if (target->isCMake())
				{
					const auto& cmakeProject = static_cast<const CMakeTarget&>(*target);
					if (!cmakeProject.runExecutable().empty())
					{
						runnableTarget = target.get();
						break;
					}
				}
			}

			if (runnableTarget != nullptr)
			{
				VSCodeLaunchGen launchJson(outState, m_cwd, *runnableTarget);
				if (!launchJson.saveToFile(fmt::format("{}/launch.json", vscodeDir)))
				{
					Diagnostic::error("There was a problem saving the launch.json file.");
					return false;
				}

				VSCodeTasksGen tasksJson(outState, m_cwd);
				if (!tasksJson.saveToFile(fmt::format("{}/tasks.json", vscodeDir)))
				{
					Diagnostic::error("There was a problem saving the tasks.json file.");
					return false;
				}
			}
		}
	}

	Commands::changeWorkingDirectory(m_cwd);

	if (state == nullptr)
	{
		Diagnostic::error("There are no valid projects to export.");
		return false;
	}

	return true;
}
}
