/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Zed/ZedDebugGen.hpp"

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
ZedDebugGen::ZedDebugGen(const ExportAdapter& inExportAdapter) :
	m_exportAdapter(inExportAdapter)
{
}

/*****************************************************************************/
bool ZedDebugGen::saveToFile(const std::string& inFilename) const
{
	Json jRoot = Json::array();

	auto& debugState = m_exportAdapter.getDebugState();
	Json configuration;
	if (!getConfiguration(configuration, debugState))
	{
		Diagnostic::error("There was an error creating the launch.json configuration: {}", inFilename);
		return false;
	}

	jRoot.emplace_back(std::move(configuration));

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
// Note: The C/C++ is not licensed to run inside of VSCodium, so we must use CodeLLDB instead
//   CodeLLDB also works on Windows with binaries generated from MSVC
//
bool ZedDebugGen::getConfiguration(Json& outJson, const BuildState& inState) const
{
	outJson = Json::object();

	if (!setCodeLLDBOptions(outJson, inState))
		return false;

	return true;
}

/*****************************************************************************/
bool ZedDebugGen::setCodeLLDBOptions(Json& outJson, const BuildState& inState) const
{
	outJson["label"] = "CodeLLDB";
	outJson["adapter"] = "CodeLLDB";
	outJson["request"] = "launch";

	// Note: stopOnEntry seems to be buggy in CodeLLDB
	//   looks like it's the entry of the runtime vs the program's entry?
	//
	outJson["stopOnEntry"] = false;

	outJson["build"] = m_exportAdapter.getAllTargetName();
	if (!setProgramPathAndArguments(outJson, inState))
		return false;

	outJson["cwd"] = getWorkingDirectory(inState);
	outJson["envFile"] = getEnvFilePath(inState);

	return true;
}

/*****************************************************************************/
bool ZedDebugGen::setProgramPathAndArguments(Json& outJson, const BuildState& inState) const
{
	constexpr bool executablesOnly = true;
	auto target = inState.getFirstValidRunTarget(executablesOnly);
	chalet_assert(target != nullptr, "no valid run targets");

	auto program = inState.paths.getExecutableTargetPath(*target);
	if (!program.empty())
	{
		outJson["program"] = fmt::format("$ZED_WORKTREE_ROOT/{}", program);
	}

	StringList arguments;
	if (!inState.getRunTargetArguments(arguments, target))
		return false;

	if (!arguments.empty())
	{
		outJson["args"] = arguments;
	}

	return true;
}

/*****************************************************************************/
std::string ZedDebugGen::getWorkingDirectory(const BuildState& inState) const
{
	constexpr bool executablesOnly = true;
	auto target = inState.getFirstValidRunTarget(executablesOnly);
	if (target != nullptr)
	{
		TargetExportAdapter adapter(inState, *target);
		return adapter.getRunWorkingDirectoryWithCurrentWorkingDirectoryAs("$ZED_WORKTREE_ROOT");
	}
	else
	{
		return std::string("$ZED_WORKTREE_ROOT");
	}
}

/*****************************************************************************/
std::string ZedDebugGen::getEnvFilePath(const BuildState& inState) const
{
	return fmt::format("$ZED_WORKTREE_ROOT/{}/run.env", inState.paths.buildOutputDir());
}

/*****************************************************************************/
bool ZedDebugGen::willUseMSVC(const BuildState& inState) const
{
	return inState.environment->isMsvc() || inState.environment->isWindowsClang();
}

/*****************************************************************************/
bool ZedDebugGen::willUseLLDB(const BuildState& inState) const
{
	return inState.environment->isClang() && !inState.environment->isWindowsClang();
}

/*****************************************************************************/
bool ZedDebugGen::willUseGDB(const BuildState& inState) const
{
	return !willUseMSVC(inState) && !willUseLLDB(inState);
}

}
