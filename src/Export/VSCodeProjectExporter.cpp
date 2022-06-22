/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSCodeProjectExporter.hpp"

#include "Export/VSCode/CCppPropertiesGen.hpp"
#include "Export/VSCode/VSCodeLaunchGen.hpp"
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
		state = m_states.begin()->second.get();
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

		VSCodeLaunchGen launchJson(outState, m_cwd);
		if (!launchJson.saveToFile(fmt::format("{}/launch.json", vscodeDir)))
		{
			Diagnostic::error("There was a problem saving the launch.json file.");
			return false;
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
