/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/ZedProjectExporter.hpp"

#include "Core/CommandLineInputs.hpp"
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

	if (!saveSchemasToDirectory(fmt::format("{}/schema", m_directory)))
		return false;

	{
		// FleetWorkspaceGen workspaceGen(*m_exportAdapter);
		// if (!workspaceGen.saveToPath(m_directory))
		// {
		// 	Diagnostic::error("There was a problem creating the Fleet workspace files.");
		// 	return false;
		// }
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
