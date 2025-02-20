/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CodeEditProjectExporter.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Export/CodeEdit/CodeEditWorkspaceGen.hpp"
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
CodeEditProjectExporter::CodeEditProjectExporter(const CommandLineInputs& inInputs) :
	IProjectExporter(inInputs, ExportKind::CodeEdit)
{
}

/*****************************************************************************/
std::string CodeEditProjectExporter::getMainProjectOutput()
{
	if (m_directory.empty())
	{
		if (!useProjectBuildDirectory(".codeedit"))
			return std::string();
	}

	return m_directory;
}

/*****************************************************************************/
std::string CodeEditProjectExporter::getProjectTypeName() const
{
	return std::string("CodeEdit");
}

/*****************************************************************************/
bool CodeEditProjectExporter::validate(const BuildState& inState)
{
	UNUSED(inState);
#if defined(CHALET_MACOS)
	return true;
#else
	auto typeName = getProjectTypeName();
	Diagnostic::error("{} project format requires CodeEdit on macOS.", typeName);
	return false;
#endif
}

/*****************************************************************************/
bool CodeEditProjectExporter::generateProjectFiles()
{
	auto output = getMainProjectOutput();
	if (output.empty())
		return false;

	// if (!saveSchemasToDirectory(fmt::format("{}/schema", m_directory)))
	// 	return false;

	{
		CodeEditWorkspaceGen workspaceGen(*m_exportAdapter);
		if (!workspaceGen.saveToPath(m_directory))
		{
			Diagnostic::error("There was a problem creating the CodeEdit workspace files.");
			return false;
		}
	}

	const auto& cwd = workingDirectory();
	auto codeEditDirectory = fmt::format("{}/.codeedit", cwd);
	if (!Files::pathExists(codeEditDirectory) && Files::pathExists(m_directory))
	{
		if (!Files::copySilent(m_directory, cwd))
		{
			Diagnostic::error("There was a problem copying the .codeedit directory to the workspace.");
			return false;
		}

		Files::removeRecursively(m_directory);
	}
	else
	{
		auto directory = m_directory;
		String::replaceAll(directory, fmt::format("{}/", cwd), "");
		Diagnostic::warn("The .codeedit directory already exists in the workspace root. Copy the files from the following directory to update them: {}", directory);
	}

	return true;
}

/*****************************************************************************/
bool CodeEditProjectExporter::openProjectFilesInEditor(const std::string& inProject)
{
	UNUSED(inProject);
#if defined(CHALET_MACOS)
	const auto& cwd = workingDirectory();
	auto shell = Environment::getShell();
	auto codeEdit = fmt::format("/Applications/CodeEdit.app");

	// This has to be called through system for some reason...
	i32 result = std::system(fmt::format("{} -c \"open {} {}\"", shell, codeEdit, cwd).c_str());
	return result == 0;
#else
	return false;
#endif
}

}
