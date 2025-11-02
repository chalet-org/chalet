/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/ZedProjectExporter.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/Zed/ZedDebugGen.hpp"
#include "Export/Zed/ZedSettingsGen.hpp"
#include "Export/Zed/ZedTasksGen.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ZedProjectExporter::ZedProjectExporter(const CommandLineInputs& inInputs) :
	IProjectExporter(inInputs, ExportKind::Zed)
{
}

/*****************************************************************************/
std::string ZedProjectExporter::getMainProjectOutput()
{
	if (m_directory.empty())
	{
		if (!useProjectBuildDirectory(".zed"))
			return std::string();
	}

	return m_directory;
}

/*****************************************************************************/
std::string ZedProjectExporter::getProjectTypeName() const
{
	return std::string("Zed");
}

/*****************************************************************************/
bool ZedProjectExporter::validate(const BuildState& inState)
{
	UNUSED(inState);

	return true;
}

/*****************************************************************************/
bool ZedProjectExporter::generateProjectFiles()
{
	auto output = getMainProjectOutput();
	if (output.empty())
		return false;

	// if (!saveSchemasToDirectory(fmt::format("{}/schema", m_directory)))
	// 	return false;

	auto& debugState = m_exportAdapter->getDebugState();

	bool allowedEnvironment = !debugState.environment->isEmscripten();
	if (debugState.configuration.debugSymbols() && allowedEnvironment)
	{
		constexpr bool executablesOnly = true;
		const IBuildTarget* runnableTarget = debugState.getFirstValidRunTarget(executablesOnly);
		if (runnableTarget != nullptr)
		{
			ZedDebugGen debugJson(*m_exportAdapter);
			if (!debugJson.saveToFile(fmt::format("{}/debug.json", m_directory)))
			{
				Diagnostic::error("There was a problem saving the debug.json file.");
				return false;
			}
		}
	}

	ZedTasksGen tasksJson(*m_exportAdapter);
	if (!tasksJson.saveToFile(fmt::format("{}/tasks.json", m_directory)))
	{
		Diagnostic::error("There was a problem saving the tasks.json file.");
		return false;
	}

	ZedSettingsGen settingsJson(debugState);
	if (!settingsJson.saveToFile(fmt::format("{}/settings.json", m_directory)))
	{
		Diagnostic::error("There was a problem saving the settings.json file.");
		return false;
	}

	if (!copyExportedDirectoryToRootWithOutput(".zed"))
		return false;

	return true;
}

/*****************************************************************************/
bool ZedProjectExporter::openProjectFilesInEditor(const std::string& inProject)
{
	UNUSED(inProject);

	const auto& cwd = workingDirectory();
	// auto project = Files::getCanonicalPath(inProject);
	auto zed = Files::which("zed");
	if (!zed.empty())
		return Process::runMinimalOutputWithoutWait({ zed, cwd });
	else
		return false;
}

}
