/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/RenderDocCaptureSettingsGen.hpp"

#include "Core/CommandLineInputs.hpp"
#include "DotEnv/DotEnvFileGenerator.hpp"
#include "Process/Environment.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
RenderDocCaptureSettingsGen::RenderDocCaptureSettingsGen(const BuildState& inState, const SourceTarget& inProject) :
	m_state(inState),
	m_project(inProject)
{}

/*****************************************************************************/
bool RenderDocCaptureSettingsGen::generate()
{
	if (!m_project.isExecutable())
	{
		Diagnostic::error("RenderDoc capture settings can only generate from an executable source target");
		return false;
	}

	auto executablePath = m_state.paths.getExecutableTargetPath(m_project);
	auto outputFilePath = fmt::format("{}.cap", String::getPathFolderBaseName(executablePath));

	executablePath = Files::getCanonicalPath(executablePath);
#if defined(CHALET_WIN32)
	Path::toWindows(executablePath);
#endif

	Json root = Json::object();
	root["rdocCaptureSettings"] = 1;
	auto& jSettings = root["settings"] = Json::object();
	jSettings["autoStart"] = false;
	jSettings["commandLine"] = getRunArguments();
	jSettings["environment"] = getEnvironmentAsJson();
	jSettings["executable"] = std::move(executablePath);
	jSettings["inject"] = false;
	jSettings["numQueuedFrames"] = 0;
	jSettings["options"] = Json::object();
	jSettings["queuedFrameCap"] = 0;
	jSettings["workingDir"] = m_state.inputs.workingDirectory();

	auto& jOptions = jSettings["options"];
	jOptions["allowFullscreen"] = true;
	jOptions["allowVSync"] = true;
	jOptions["apiValidation"] = false;
	jOptions["captureAllCmdLists"] = false;
	jOptions["captureCallstacks"] = false;
	jOptions["captureCallstacksOnlyDraws"] = false;
	jOptions["debugOutputMute"] = true;
	jOptions["delayForDebugger"] = 0;
	jOptions["hookIntoChildren"] = false;
	jOptions["refAllResources"] = false;
	jOptions["softMemoryLimit"] = 0;
	jOptions["verifyBufferAccess"] = false;

	if (!JsonFile::saveToFile(root, outputFilePath))
	{
		Diagnostic::error("RenderDoc capture settings failed to save for: {}", m_project.name());
		return false;
	}

	return true;
}

/*****************************************************************************/
Json RenderDocCaptureSettingsGen::getEnvironmentAsJson() const
{
	Json result = Json::array();

	const auto& env = m_state.getEnvironmentVariables();

	{
		auto& jPath = result.emplace_back(Json::object());
		jPath["separator"] = "Platform style";
		jPath["type"] = "Prepend";
		jPath["value"] = env.getRunPaths();
		jPath["variable"] = "Path";
	}

	auto& runEnvVars = env.variables();
	for (const auto& [key, value] : runEnvVars)
	{
		if (String::startsWith("__", key) || String::equals("PATH", key))
			continue;

		auto& jPath = result.emplace_back(Json::object());
		jPath["separator"] = "Platform style";
		jPath["type"] = "Set";
		jPath["value"] = value;
		jPath["variable"] = key;
	}

	return result;
}

/*****************************************************************************/
std::string RenderDocCaptureSettingsGen::getRunArguments() const
{
	StringList result;
	m_state.getRunTargetArguments(result, &m_project);
	return String::join(result);
}
}
