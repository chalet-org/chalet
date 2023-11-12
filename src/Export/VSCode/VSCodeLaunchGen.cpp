/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSCode/VSCodeLaunchGen.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
VSCodeLaunchGen::VSCodeLaunchGen(const BuildState& inState, const IBuildTarget& inTarget) :
	m_state(inState),
	m_target(inTarget)
{
}

/*****************************************************************************/
bool VSCodeLaunchGen::saveToFile(const std::string& inFilename) const
{
	Json jRoot;
	jRoot = Json::object();
	jRoot["version"] = "0.2.0";
	jRoot["configurations"] = Json::array();
	auto& configurations = jRoot.at("configurations");

	configurations.push_back(getConfiguration());

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
Json VSCodeLaunchGen::getConfiguration() const
{
	Json ret = Json::object();
	ret["name"] = getName();
	ret["type"] = getType();
	ret["request"] = "launch";
	ret["stopAtEntry"] = true;
	ret["cwd"] = "${workspaceFolder}";

	setOptions(ret);
	setPreLaunchTask(ret);
	setProgramPath(ret);
	setEnvFilePath(ret);

	return ret;
}

/*****************************************************************************/
std::string VSCodeLaunchGen::getName() const
{
	if (willUseLLDB())
		return std::string("LLDB");
	else if (willUseMSVC())
		return std::string("MSVC");
	else
		return std::string("GDB");
}

/*****************************************************************************/
std::string VSCodeLaunchGen::getType() const
{
	if (willUseMSVC())
		return std::string("cppvsdbg");
	else
		return std::string("cppdbg");
}

/*****************************************************************************/
std::string VSCodeLaunchGen::getDebuggerPath() const
{
	const bool isLLDB = willUseLLDB();
	if (isLLDB || willUseGDB())
	{
		auto debugger = isLLDB ? "lldb" : "gdb";
		auto compilerPath = String::getPathFolder(m_state.toolchain.compilerCxxAny().path);
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
void VSCodeLaunchGen::setOptions(Json& outJson) const
{
	if (willUseMSVC())
	{
		outJson["console"] = "integratedTerminal";
	}
	else
	{
		outJson["externalConsole"] = false;
		outJson["internalConsoleOptions"] = "neverOpen";

		const bool isLLDB = willUseLLDB();
		outJson["MIMode"] = isLLDB ? "lldb" : "gdb";

#if defined(CHALET_MACOS)
		if (!isLLDB)
#endif
		{
			outJson["miDebuggerPath"] = getDebuggerPath();
		}
	}
}

/*****************************************************************************/
void VSCodeLaunchGen::setPreLaunchTask(Json& outJson) const
{
	outJson["preLaunchTask"] = fmt::format("Build: {}", m_state.configuration.name());
}

/*****************************************************************************/
void VSCodeLaunchGen::setProgramPath(Json& outJson) const
{
	auto program = m_state.paths.getExecutableTargetPath(m_target);
	if (!program.empty())
	{
		outJson["program"] = fmt::format("${{workspaceFolder}}/{}", program);
	}

	if (m_state.inputs.runArguments().has_value())
	{
		outJson["args"] = *m_state.inputs.runArguments();
	}
	else
	{
		outJson["args"] = Json::array();
	}
}

/*****************************************************************************/
void VSCodeLaunchGen::setEnvFilePath(Json& outJson) const
{
	outJson["envFile"] = fmt::format("${{workspaceFolder}}/{}/run.env", m_state.paths.buildOutputDir());
}

/*****************************************************************************/
bool VSCodeLaunchGen::willUseMSVC() const
{
	return m_state.environment->isMsvc() || m_state.environment->isWindowsClang();
}

/*****************************************************************************/
bool VSCodeLaunchGen::willUseLLDB() const
{
	return m_state.environment->isClang() && !m_state.environment->isWindowsClang();
}

/*****************************************************************************/
bool VSCodeLaunchGen::willUseGDB() const
{
	return !willUseMSVC() && !willUseLLDB();
}

}
