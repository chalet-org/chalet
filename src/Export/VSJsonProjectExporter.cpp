/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSJsonProjectExporter.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Export/VisualStudio/VSCppPropertiesGen.hpp"
#include "Export/VisualStudio/VSLaunchGen.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
/*****************************************************************************/
VSJsonProjectExporter::VSJsonProjectExporter(CentralState& inCentralState) :
	IProjectExporter(inCentralState, ExportKind::VisualStudioJSON)
{
}

/*****************************************************************************/
std::string VSJsonProjectExporter::getProjectTypeName() const
{
	return std::string("Visual Studio JSON");
}

/*****************************************************************************/
bool VSJsonProjectExporter::validate(const BuildState& inState)
{
	UNUSED(inState);

	return true;
}

/*****************************************************************************/
bool VSJsonProjectExporter::generateProjectFiles()
{
	if (!useExportDirectory("vs-json"))
		return false;

	const BuildState* state = getAnyBuildStateButPreferDebug();
	chalet_assert(state != nullptr, "");
	if (state != nullptr)
	{
		const auto& outState = *state;
		VSCppPropertiesGen cppProperties(outState, m_cwd);
		if (!cppProperties.saveToFile("CppProperties.json"))
		{
			Diagnostic::error("There was a problem saving the CppProperties.json file.");
			return false;
		}

		if (state->configuration.debugSymbols())
		{
			const IBuildTarget* runnableTarget = getRunnableTarget(*state);
			if (runnableTarget != nullptr)
			{
				VSLaunchGen launchJson(outState, m_cwd, *runnableTarget);
				if (!launchJson.saveToFile("launch.vs.json"))
				{
					Diagnostic::error("There was a problem saving the launch.vs.json file.");
					return false;
				}
			}
		}
	}

	return true;
}
}
