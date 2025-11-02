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
VSCodeLaunchGen::VSCodeLaunchGen(const ExportAdapter& inExportAdapter, const VSCodeExtensionAwarenessAdapter& inExtensionAdapter) :
	m_exportAdapter(inExportAdapter),
	m_extensionAdapter(inExtensionAdapter)
{}

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
// Note: The C/C++ is not licensed to run inside of VSCodium, so we must use CodeLLDB instead
//   CodeLLDB also works on Windows with binaries generated from MSVC
//
bool VSCodeLaunchGen::getConfiguration(Json& outJson, const BuildState& inState) const
{
	outJson = Json::object();

	if (m_extensionAdapter.cppToolsExtensionInstalled())
	{
		if (!setCppToolsDebugOptions(outJson, inState))
			return false;
	}
	else
	{
		if (!setCodeLLDBOptions(outJson, inState))
			return false;
	}

	return true;
}

/*****************************************************************************/
std::string VSCodeLaunchGen::getCppToolsDebugName(const BuildState& inState) const
{
	if (willUseLLDB(inState))
		return std::string("LLDB");
	else if (willUseMSVC(inState))
		return std::string("MSVC");
	else
		return std::string("GDB");
}

/*****************************************************************************/
std::string VSCodeLaunchGen::getCppToolsDebugType(const BuildState& inState) const
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
bool VSCodeLaunchGen::setCodeLLDBOptions(Json& outJson, const BuildState& inState) const
{
	outJson["name"] = "CodeLLDB";
	outJson["type"] = "lldb";
	outJson["request"] = "launch";

	// Note: stopOnEntry seems to be buggy in CodeLLDB
	//   looks like it's the entry of the runtime vs the program's entry?
	//
	outJson["stopOnEntry"] = false;

	outJson["console"] = "integratedTerminal";

	setPreLaunchTask(outJson);
	if (!setProgramPathAndArguments(outJson, inState))
		return false;

	outJson["cwd"] = getWorkingDirectory(inState);
	outJson["envFile"] = getEnvFilePath(inState);

	return true;
}

/*****************************************************************************/
bool VSCodeLaunchGen::setCppToolsDebugOptions(Json& outJson, const BuildState& inState) const
{
	outJson["name"] = getCppToolsDebugName(inState);
	outJson["type"] = getCppToolsDebugType(inState);
	outJson["request"] = "launch";
	outJson["stopAtEntry"] = true;

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

	setPreLaunchTask(outJson);
	if (!setProgramPathAndArguments(outJson, inState))
		return false;

	outJson["cwd"] = getWorkingDirectory(inState);
	outJson["envFile"] = getEnvFilePath(inState);

	return true;
}

/*****************************************************************************/
void VSCodeLaunchGen::setPreLaunchTask(Json& outJson) const
{
	outJson["preLaunchTask"] = m_exportAdapter.getAllTargetName();
}

/*****************************************************************************/
bool VSCodeLaunchGen::setProgramPathAndArguments(Json& outJson, const BuildState& inState) const
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
std::string VSCodeLaunchGen::getWorkingDirectory(const BuildState& inState) const
{
	constexpr bool executablesOnly = true;
	auto target = inState.getFirstValidRunTarget(executablesOnly);
	if (target != nullptr)
	{
		TargetExportAdapter adapter(inState, *target);
		return adapter.getRunWorkingDirectoryWithCurrentWorkingDirectoryAs("${workspaceFolder}");
	}
	else
	{
		return std::string("${workspaceFolder}");
	}
}

/*****************************************************************************/
std::string VSCodeLaunchGen::getEnvFilePath(const BuildState& inState) const
{
	return fmt::format("${{workspaceFolder}}/{}/run.env", inState.paths.buildOutputDir());
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
