/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSCodeProjectExporter.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/ExportAdapter.hpp"
#include "Export/VSCode/VSCodeCCppPropertiesGen.hpp"
#include "Export/VSCode/VSCodeLaunchGen.hpp"
#include "Export/VSCode/VSCodeSettingsGen.hpp"
#include "Export/VSCode/VSCodeTasksGen.hpp"
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
VSCodeProjectExporter::VSCodeProjectExporter(const CommandLineInputs& inInputs) :
	IProjectExporter(inInputs, inInputs.exportKind()),
	m_vscodium(inInputs.exportKind() == ExportKind::VSCodiumJSON)
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
	if (m_vscodium)
		return std::string("VSCodium");
	else
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

	auto& debugState = m_exportAdapter->getDebugState();
	VSCodeCCppPropertiesGen cCppProperties(debugState, *m_exportAdapter);
	if (!cCppProperties.saveToFile(fmt::format("{}/c_cpp_properties.json", m_directory)))
	{
		Diagnostic::error("There was a problem saving the c_cpp_properties.json file.");
		return false;
	}

	bool allowedEnvironment = !debugState.environment->isEmscripten();
	if (debugState.configuration.debugSymbols() && allowedEnvironment)
	{
		constexpr bool executablesOnly = true;
		const IBuildTarget* runnableTarget = debugState.getFirstValidRunTarget(executablesOnly);
		if (runnableTarget != nullptr)
		{
			VSCodeLaunchGen launchJson(*m_exportAdapter, m_vscodium);
			if (!launchJson.saveToFile(fmt::format("{}/launch.json", m_directory)))
			{
				Diagnostic::error("There was a problem saving the launch.json file.");
				return false;
			}
		}
	}

	VSCodeTasksGen tasksJson(*m_exportAdapter);
	if (!tasksJson.saveToFile(fmt::format("{}/tasks.json", m_directory)))
	{
		Diagnostic::error("There was a problem saving the tasks.json file.");
		return false;
	}

	auto clangFormat = fmt::format("{}/.clang-format", debugState.inputs.workingDirectory());
	if (Files::pathExists(clangFormat))
	{
		VSCodeSettingsGen settingsJson(debugState);
		if (!settingsJson.saveToFile(fmt::format("{}/settings.json", m_directory)))
		{
			Diagnostic::error("There was a problem saving the settings.json file.");
			return false;
		}
	}

	if (!copyExportedDirectoryToRootWithOutput(".vscode"))
		return false;

	return true;
}

/*****************************************************************************/
bool VSCodeProjectExporter::openProjectFilesInEditor(const std::string& inProject)
{
	UNUSED(inProject);

	const auto& cwd = workingDirectory();
	auto codeShell = m_vscodium ? "codium" : "code";

	// auto project = Files::getCanonicalPath(inProject);
	auto code = Files::which(codeShell);
#if defined(CHALET_WIN32)
	if (code.empty())
	{
		if (m_vscodium)
		{
			auto programFiles = Environment::getProgramFiles();
			code = Files::getCanonicalPath(fmt::format("{}/VSCodium/VSCodium.exe", programFiles));
		}
		else
		{
			auto appData = Environment::get("APPDATA");
			code = Files::getCanonicalPath(fmt::format("{}/../Local/Programs/Microsoft VS Code/Code.exe", appData));
		}

		if (!Files::pathExists(code))
			code.clear();
	}
#endif
	if (!code.empty())
		return Process::runMinimalOutputWithoutWait({ code, cwd });
	else
		return false;
}
}
