/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSCode/VSCodeLaunchGen.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/TargetExportAdapter.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
VSCodeLaunchGen::VSCodeLaunchGen(const ExportAdapter& inExportAdapter) :
	m_exportAdapter(inExportAdapter)
{
}

/*****************************************************************************/
bool VSCodeLaunchGen::saveToFile(const std::string& inFilename) const
{
	Json jRoot = Json::object();
	jRoot["version"] = "0.2.0";
	auto& configurations = jRoot["configurations"] = Json::array();

	auto& debugState = m_exportAdapter.getDebugState();
	Json configuration;
	if (!getConfiguration(configuration, debugState))
	{
		Diagnostic::error("There was an error creating the launch.json configuration: {}", inFilename);
		return false;
	}

	configurations.emplace_back(std::move(configuration));

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
bool VSCodeLaunchGen::getConfiguration(Json& outConfiguration, const BuildState& inState) const
{
	outConfiguration = Json::object();
	outConfiguration["name"] = getName(inState);
	outConfiguration["type"] = getType(inState);
	outConfiguration["request"] = "launch";
	outConfiguration["stopAtEntry"] = true;

	setOptions(outConfiguration, inState);
	setPreLaunchTask(outConfiguration);
	if (!setProgramPath(outConfiguration, inState))
		return false;

	setWorkingDirectory(outConfiguration, inState);
	setEnvFilePath(outConfiguration, inState);

	return true;
}

/*****************************************************************************/
std::string VSCodeLaunchGen::getName(const BuildState& inState) const
{
	if (willUseLLDB(inState))
		return std::string("LLDB");
	else if (willUseMSVC(inState))
		return std::string("MSVC");
	else
		return std::string("GDB");
}

/*****************************************************************************/
std::string VSCodeLaunchGen::getType(const BuildState& inState) const
{
	if (willUseMSVC(inState))
		return std::string("cppvsdbg");
	else
		return std::string("cppdbg");
}

/*****************************************************************************/
std::string VSCodeLaunchGen::getDebuggerPath(const BuildState& inState) const
{
	const bool isLLDB = willUseLLDB(inState);
	if (isLLDB || willUseGDB(inState))
	{
		auto& debugState = m_exportAdapter.getDebugState();
		auto debugger = isLLDB ? "lldb" : "gdb";
		auto compilerPath = String::getPathFolder(debugState.toolchain.compilerCxxAny().path);
		auto exe = Files::getPlatformExecutableExtension();
		auto path = fmt::format("{}/{}{}", compilerPath, debugger, exe);
		if (!Files::pathExists(path))
		{
			path = Files::which(debugger);
		}

		return path;
	}

	return std::string();
}

/*****************************************************************************/
void VSCodeLaunchGen::setOptions(Json& outJson, const BuildState& inState) const
{
	if (willUseMSVC(inState))
	{
		outJson["console"] = "integratedTerminal";
	}
	else
	{
		outJson["externalConsole"] = false;
		outJson["internalConsoleOptions"] = "neverOpen";

		const bool isLLDB = willUseLLDB(inState);
		outJson["MIMode"] = isLLDB ? "lldb" : "gdb";

#if defined(CHALET_MACOS)
		if (!isLLDB)
#endif
		{
			outJson["miDebuggerPath"] = getDebuggerPath(inState);
		}
	}
}

/*****************************************************************************/
void VSCodeLaunchGen::setPreLaunchTask(Json& outJson) const
{
	outJson["preLaunchTask"] = m_exportAdapter.getAllTargetName();
}

/*****************************************************************************/
bool VSCodeLaunchGen::setProgramPath(Json& outJson, const BuildState& inState) const
{
	constexpr bool executablesOnly = true;
	auto target = inState.getFirstValidRunTarget(executablesOnly);
	chalet_assert(target != nullptr, "no valid run targets");

	auto program = inState.paths.getExecutableTargetPath(*target);
	if (!program.empty())
	{
		outJson["program"] = fmt::format("${{workspaceFolder}}/{}", program);
	}

	StringList arguments;
	if (!inState.getRunTargetArguments(arguments, target))
		return false;

	outJson["args"] = arguments;

	return true;
}

/*****************************************************************************/
bool VSCodeLaunchGen::setWorkingDirectory(Json& outJson, const BuildState& inState) const
{
	constexpr bool executablesOnly = true;
	auto target = inState.getFirstValidRunTarget(executablesOnly);
	if (target != nullptr)
	{
		TargetExportAdapter adapter(inState, *target);
		outJson["cwd"] = adapter.getRunWorkingDirectoryWithCurrentWorkingDirectoryAs("${workspaceFolder}");
	}
	else
	{
		outJson["cwd"] = "${workspaceFolder}";
	}

	return true;
}

/*****************************************************************************/
void VSCodeLaunchGen::setEnvFilePath(Json& outJson, const BuildState& inState) const
{
	outJson["envFile"] = fmt::format("${{workspaceFolder}}/{}/run.env", inState.paths.buildOutputDir());
}

/*****************************************************************************/
bool VSCodeLaunchGen::willUseMSVC(const BuildState& inState) const
{
	return inState.environment->isMsvc() || inState.environment->isWindowsClang();
}

/*****************************************************************************/
bool VSCodeLaunchGen::willUseLLDB(const BuildState& inState) const
{
	return inState.environment->isClang() && !inState.environment->isWindowsClang();
}

/*****************************************************************************/
bool VSCodeLaunchGen::willUseGDB(const BuildState& inState) const
{
	return !willUseMSVC(inState) && !willUseLLDB(inState);
}

}
