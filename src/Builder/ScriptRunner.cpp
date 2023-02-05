/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/ScriptRunner.hpp"

#include "Core/CommandLineInputs.hpp"

#include "Process/ProcessController.hpp"
#include "State/AncillaryTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ScriptRunner::ScriptRunner(const CommandLineInputs& inInputs, const AncillaryTools& inTools) :
	m_inputs(inInputs),
	m_tools(inTools),
	m_inputFile(m_inputs.inputFile())
{
}

/*****************************************************************************/
bool ScriptRunner::run(const ScriptType inType, const std::string& inScript, const StringList& inArguments, const bool inShowExitCode)
{
	auto command = getCommand(inType, inScript, inArguments);
	if (command.empty())
		return false;

	bool result = Commands::subprocess(command);
	auto exitCode = ProcessController::getLastExitCode();

	// std::string script = inScript;
	// m_inputs.clearWorkingDirectory(script);

	auto message = fmt::format("{} exited with code: {}", inScript, exitCode);
	if (inShowExitCode)
	{
		Output::printSeparator();
		Output::print(result ? Output::theme().info : Output::theme().error, message);
	}
	else if (!result)
	{
		Diagnostic::error("{}", message);
	}

	// LOG("exitWithCode: ", inShowExitCode);

	return result;
}

/*****************************************************************************/
StringList ScriptRunner::getCommand(const ScriptType inType, const std::string& inScript, const StringList& inArguments)
{
	StringList ret;

	const auto& executable = m_tools.scriptAdapter().getExecutable(inType);
	if (executable.empty())
		return ret;

	ret.emplace_back(executable);

	if (inType == ScriptType::WindowsCommand)
	{
		ret.emplace_back("/c");
	}
	else if (inType == ScriptType::Tcl)
	{
		ret.emplace_back("-encoding");
		ret.emplace_back("utf-8");
	}
	else if (inType == ScriptType::Awk)
	{
		ret.emplace_back("-f");
	}

	ret.emplace_back(inScript);

	for (const auto& arg : inArguments)
	{
		ret.emplace_back(arg);
	}

	return ret;
}
}
