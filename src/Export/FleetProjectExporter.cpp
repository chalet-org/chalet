/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/FleetProjectExporter.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Export/Fleet/FleetWorkspaceGen.hpp"
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
FleetProjectExporter::FleetProjectExporter(const CommandLineInputs& inInputs) :
	IProjectExporter(inInputs, ExportKind::Fleet)
{
}

/*****************************************************************************/
std::string FleetProjectExporter::getMainProjectOutput()
{
	if (m_directory.empty())
	{
		if (!useProjectBuildDirectory(".fleet"))
			return std::string();
	}

	return m_directory;
}

/*****************************************************************************/
std::string FleetProjectExporter::getProjectTypeName() const
{
	return std::string("Fleet");
}

/*****************************************************************************/
bool FleetProjectExporter::validate(const BuildState& inState)
{
	UNUSED(inState);

	return true;
}

/*****************************************************************************/
bool FleetProjectExporter::generateProjectFiles()
{
	auto output = getMainProjectOutput();
	if (output.empty())
		return false;

	if (!saveSchemasToDirectory(fmt::format("{}/schema", m_directory)))
		return false;

	{
		FleetWorkspaceGen workspaceGen(*m_exportAdapter);
		if (!workspaceGen.saveToPath(m_directory))
		{
			Diagnostic::error("There was a problem creating the Fleet workspace files.");
			return false;
		}
	}

	const auto& cwd = workingDirectory();
	auto ideaDirectory = fmt::format("{}/.fleet", cwd);
	if (!Files::pathExists(ideaDirectory) && Files::pathExists(m_directory))
	{
		if (!Files::copySilent(m_directory, cwd))
		{
			Diagnostic::error("There was a problem copying the .fleet directory to the workspace.");
			return false;
		}
	}
	else
	{
		auto directory = m_directory;
		String::replaceAll(directory, fmt::format("{}/", cwd), "");
		Diagnostic::warn("The .fleet directory already exists in the workspace root. Copy the files from the following directory to update them: {}", directory);
	}

	return true;
}

/*****************************************************************************/
bool FleetProjectExporter::openProjectFilesInEditor(const std::string& inProject)
{
	UNUSED(inProject);
	const auto& cwd = workingDirectory();

	// auto project = Files::getCanonicalPath(inProject);
	auto fleet = Files::which("fleet");
#if defined(CHALET_WIN32)
	if (fleet.empty())
	{
		auto appData = Environment::get("APPDATA");
		fleet = Files::getCanonicalPath(fmt::format("{}/../Local/Programs/Fleet/Fleet.exe", appData));
		if (!Files::pathExists(fleet))
			fleet.clear();
	}
#endif
	if (!fleet.empty())
		return Process::runMinimalOutputWithoutWait({ fleet, cwd });
	else
		return false;
}

}
