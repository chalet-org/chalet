/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSJsonProjectExporter.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/VisualStudioJson/VSCppPropertiesGen.hpp"
#include "Export/VisualStudioJson/VSLaunchGen.hpp"
#include "Export/VisualStudioJson/VSTasksGen.hpp"
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
VSJsonProjectExporter::VSJsonProjectExporter(const CommandLineInputs& inInputs) :
	IProjectExporter(inInputs, ExportKind::VisualStudioJSON)
{
}

/*****************************************************************************/
std::string VSJsonProjectExporter::getMainProjectOutput()
{
	if (m_directory.empty())
	{
		if (!useProjectBuildDirectory(".vs"))
			return std::string();
	}

	return m_directory;
}
/*****************************************************************************/
std::string VSJsonProjectExporter::getProjectTypeName() const
{
	return std::string("Visual Studio JSON");
}

/*****************************************************************************/
bool VSJsonProjectExporter::validate(const BuildState& inState)
{
	auto typeName = getProjectTypeName();
	if (!inState.environment->isMsvc())
	{
#if defined(CHALET_WIN32)
		Diagnostic::error("{} project format requires the Visual Studio toolchain (set with --toolchain/-t).", typeName);
#else
		Diagnostic::error("{} project format requires the Visual Studio toolchain on Windows.", typeName);
#endif
		return false;
	}

	return true;
}

/*****************************************************************************/
bool VSJsonProjectExporter::generateProjectFiles()
{
	auto output = getMainProjectOutput();
	if (output.empty())
		return false;

	// if (!saveSchemasToDirectory(fmt::format("{}/schema", m_directory)))
	// 	return false;

	VSCppPropertiesGen cppProperties(*m_exportAdapter);
	if (!cppProperties.saveToFile(fmt::format("{}/CppProperties.json", m_directory)))
	{
		Diagnostic::error("There was a problem saving the CppProperties.json file.");
		return false;
	}

	VSTasksGen tasksJson(*m_exportAdapter);
	if (!tasksJson.saveToFile(fmt::format("{}/tasks.vs.json", m_directory)))
	{
		Diagnostic::error("There was a problem saving the tasks.vs.json file.");
		return false;
	}

	auto& debugState = m_exportAdapter->getDebugState();
	if (debugState.configuration.debugSymbols())
	{
		VSLaunchGen launchJson(*m_exportAdapter);
		if (!launchJson.saveToFile(fmt::format("{}/launch.vs.json", m_directory)))
		{
			Diagnostic::error("There was a problem saving the launch.vs.json file.");
			return false;
		}
	}

	if (!copyExportedDirectoryToRootWithOutput(".vs"))
		return false;

	return true;
}

/*****************************************************************************/
bool VSJsonProjectExporter::openProjectFilesInEditor(const std::string& inProject)
{
	const auto& cwd = workingDirectory();
	std::string devEnvDir;
	auto visualStudio = Files::which("devenv");
	if (visualStudio.empty())
	{
		devEnvDir = Environment::getString("DevEnvDir");
		visualStudio = fmt::format("{}\\devenv.exe", devEnvDir);
		if (devEnvDir.empty() || !Files::pathExists(visualStudio))
		{
			Diagnostic::error("Failed to launch in Visual Studio: {}", inProject);
			return false;
		}
	}
	else
	{
		devEnvDir = String::getPathFolder(visualStudio);
	}

	// auto project = Files::getCanonicalPath(inProject);
	return Process::runMinimalOutputWithoutWait({ visualStudio, cwd }, devEnvDir);

	// UNUSED(inProject);
	// return true;
}
}
