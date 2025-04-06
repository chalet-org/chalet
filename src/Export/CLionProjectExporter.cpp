/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CLionProjectExporter.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Export/CLion/CLionWorkspaceGen.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "System/Files.hpp"
#include "Utility/Path.hpp"
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

	if (!saveSchemasToDirectory(fmt::format("{}/schema", m_directory)))
		return false;

	{
		CLionWorkspaceGen workspaceGen(*m_exportAdapter);
		if (!workspaceGen.saveToPath(m_directory))
		{
			Diagnostic::error("There was a problem creating the CLion workspace files.");
			return false;
		}
	}

	if (!copyExportedDirectoryToRootWithOutput(".idea"))
		return false;

	return true;
}

/*****************************************************************************/
bool CLionProjectExporter::openProjectFilesInEditor(const std::string& inProject)
{
	UNUSED(inProject);

	// auto project = Files::getCanonicalPath(inProject);
	auto clion = Files::which("clion");
#if defined(CHALET_WIN32)
	if (clion.empty())
		clion = Files::which("clion64");

	if (clion.empty())
	{
		auto programs = Environment::getProgramFiles();
		if (!programs.empty())
		{
			Path::toUnix(programs);
			auto jetbrains = fmt::format("{}/JetBrains", programs);

			// We need a safe way to check for CLion directories, and get the latest version
			// Can't really

			std::map<std::string, std::string> directories;

			CHALET_TRY
			{
				for (auto& it : fs::directory_iterator(jetbrains))
				{
					if (it.is_directory())
					{
						auto folder = String::toLowerCase(it.path().filename().string());
						if (String::startsWith("clion", folder))
						{
							auto path = it.path().string();
							Path::toUnix(path);
							directories.emplace(folder, std::move(path));
						}
					}
				}
			}
			CHALET_CATCH(const std::exception&)
			{
				return false;
			}

			if (!directories.empty())
			{
				const auto& clionDirectory = directories.rbegin()->second;
				clion = fmt::format("{}/bin/clion64.exe", clionDirectory);
				if (!Files::pathExists(clion))
					clion.clear();
			}
		}
	}
#endif

	if (!clion.empty())
		return Process::runMinimalOutputWithoutWait({ clion, workingDirectory() });
	else
		return false;
}
}
