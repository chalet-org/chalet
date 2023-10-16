/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CLionProjectExporter.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Export/CLion/CLionWorkspaceGen.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CLionProjectExporter::CLionProjectExporter(const CommandLineInputs& inInputs) :
	IProjectExporter(inInputs, ExportKind::CLion)
{
}

/*****************************************************************************/
std::string CLionProjectExporter::getMainProjectOutput()
{
	if (m_directory.empty())
	{
		if (!useProjectBuildDirectory(".idea"))
			return std::string();
	}

	return m_directory;
}

/*****************************************************************************/
std::string CLionProjectExporter::getProjectTypeName() const
{
	return std::string("CLion");
}

/*****************************************************************************/
bool CLionProjectExporter::validate(const BuildState& inState)
{
	UNUSED(inState);

	return true;
}

/*****************************************************************************/
bool CLionProjectExporter::generateProjectFiles()
{
	auto output = getMainProjectOutput();
	if (output.empty())
		return false;

	{
		auto allBuildTargetName = getAllBuildTargetName();
		CLionWorkspaceGen workspaceGen(m_states, m_debugConfiguration, allBuildTargetName);
		if (!workspaceGen.saveToPath(m_directory))
		{
			Diagnostic::error("There was a problem creating the CLion workspace files.");
			return false;
		}
	}

	const auto& cwd = workingDirectory();
	auto ideaDirectory = fmt::format("{}/.idea", cwd);
	if (!Commands::pathExists(ideaDirectory) && Commands::pathExists(m_directory))
	{
		if (!Commands::copySilent(m_directory, cwd))
		{
			Diagnostic::error("There was a problem copying the .idea directory to the workspace.");
			return false;
		}
	}
	else
	{
		auto directory = m_directory;
		String::replaceAll(directory, fmt::format("{}/", cwd), "");
		Diagnostic::warn("The .idea directory already exists in the workspace root. Copy the files from the following directory to update them: {}", directory);
	}

	return true;
}
}
