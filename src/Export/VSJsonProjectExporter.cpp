/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSJsonProjectExporter.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/VisualStudio/VSCppPropertiesGen.hpp"
#include "Export/VisualStudio/VSLaunchGen.hpp"
#include "Export/VisualStudio/VSTasksGen.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"

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
		if (!useProjectBuildDirectory(".vsjson"))
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

	const BuildState* state = getAnyBuildStateButPreferDebug();
	chalet_assert(state != nullptr, "");
	if (state != nullptr)
	{
		const auto& outState = *state;

		VSCppPropertiesGen cppProperties(m_states, m_pathVariables);
		if (!cppProperties.saveToFile(fmt::format("{}/CppProperties.json", m_directory)))
		{
			Diagnostic::error("There was a problem saving the CppProperties.json file.");
			return false;
		}

		VSTasksGen tasksJson(outState);
		if (!tasksJson.saveToFile(fmt::format("{}/tasks.vs.json", m_directory)))
		{
			Diagnostic::error("There was a problem saving the tasks.vs.json file.");
			return false;
		}

		if (state->configuration.debugSymbols())
		{
			VSLaunchGen launchJson(m_states);
			if (!launchJson.saveToFile(fmt::format("{}/launch.vs.json", m_directory)))
			{
				Diagnostic::error("There was a problem saving the launch.vs.json file.");
				return false;
			}
		}
	}

	return true;
}
}
