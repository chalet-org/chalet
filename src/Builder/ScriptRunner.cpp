/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/ScriptRunner.hpp"

#include "Core/CommandLineInputs.hpp"

#include "Process/ProcessController.hpp"
#include "State/AncillaryTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
/*****************************************************************************/
std::string getScriptTypeString(const ScriptType inType)
{
	switch (inType)
	{
		case ScriptType::Python:
			return "python";
		case ScriptType::Ruby:
			return "ruby";
		case ScriptType::Perl:
			return "perl";
		case ScriptType::Lua:
			return "lua";
		case ScriptType::UnixShell:
		case ScriptType::Powershell:
		case ScriptType::WindowsCommand:
		case ScriptType::None:
		default:
			return std::string();
	}
}

/*****************************************************************************/
ScriptType getScriptTypeFromString(const std::string& inStr)
{
	auto lower = String::toLowerCase(inStr);
	if (String::equals({ "python", "python3" }, lower))
		return ScriptType::Python;
	if (String::equals("lua", lower))
		return ScriptType::Lua;
	if (String::equals("ruby", lower))
		return ScriptType::Ruby;
	if (String::equals("perl", lower))
		return ScriptType::Perl;
	if (String::equals("pwsh", lower))
		return ScriptType::Powershell;

	return ScriptType::UnixShell;
}
}

/*****************************************************************************/
ScriptRunner::ScriptRunner(const CommandLineInputs& inInputs, const AncillaryTools& inTools) :
	m_inputs(inInputs),
	m_tools(inTools),
	m_inputFile(m_inputs.inputFile())
{
}

/*****************************************************************************/
bool ScriptRunner::run(const std::string& inScript, const StringList& inArguments, const bool inShowExitCode)
{
	auto [command, _] = getCommand(inScript, inArguments);
	if (command.empty())
		return false;

	bool result = Commands::subprocess(command);
	auto exitCode = ProcessController::getLastExitCode();

	std::string script = inScript;
	m_inputs.clearWorkingDirectory(script);

	auto message = fmt::format("{} exited with code: {}", inScript, exitCode);
	if (inShowExitCode || !result)
	{
		if (inShowExitCode)
			Output::printSeparator();
		else
			Output::lineBreak();

		Output::print(result ? Output::theme().info : Output::theme().error, message);
	}

	return result;
}

/*****************************************************************************/
std::pair<StringList, ScriptType> ScriptRunner::getCommand(const std::string& inScript, const StringList& inArguments)
{
	StringList ret;

#if defined(CHALET_WIN32)
	std::string parsedScriptPath = inScript;
	if (String::endsWith(".exe", parsedScriptPath))
		parsedScriptPath = parsedScriptPath.substr(0, parsedScriptPath.size() - 4);

	auto outScriptPath = Commands::which(parsedScriptPath);
#else
	auto outScriptPath = Commands::which(inScript);
#endif
	if (outScriptPath.empty())
		outScriptPath = fs::absolute(inScript).string();

	if (!Commands::pathExists(outScriptPath))
	{
		Diagnostic::error("{}: The script '{}' was not found. Aborting.", m_inputFile, inScript);
		return std::make_pair(ret, ScriptType::None);
	}

	Commands::setExecutableFlag(outScriptPath);

	std::string shebang;
	bool shellFound = false;
	ScriptType scriptType = ScriptType::None;

	std::string shell;
	shebang = Commands::readShebangFromFile(outScriptPath);
	if (!shebang.empty())
	{
		if (String::startsWith("/usr/bin/env ", shebang))
		{
			auto space = shebang.find(' ');
			auto search = shebang.substr(space + 1);
			if (!search.empty())
			{
				scriptType = getScriptTypeFromString(search);
				shell = Commands::which(shebang.substr(space + 1));
				shellFound = !shell.empty();
			}
		}
		else
		{
			auto search = String::getPathFilename(shebang);
			if (!search.empty())
			{
				scriptType = getScriptTypeFromString(search);

				shell = shebang;
				shellFound = Commands::pathExists(shell);

				if (!shellFound)
				{
					shell = Commands::which(search);
					shellFound = !shell.empty();

					if (!shellFound)
					{
						shell = Environment::getShell();
						shellFound = !shell.empty();
					}
				}
			}
		}
	}

	if (!shellFound)
	{
		if (String::endsWith(StringList{ ".sh", ".bash" }, outScriptPath))
		{
			scriptType = ScriptType::UnixShell;

			shell = Environment::getShell();
			shellFound = !shell.empty();

			if (!shellFound && !m_tools.bash().empty())
			{
				shell = m_tools.bash();
				shellFound = true;
			}
		}
		else if (String::endsWith(".py", outScriptPath))
		{
			scriptType = ScriptType::Python;

			auto python = Commands::which("python3");
			if (!python.empty())
			{
				shell = std::move(python);
				shellFound = true;
			}
			else
			{
				python = Commands::which("python");
				if (!python.empty())
				{
					shell = std::move(python);
					shellFound = true;
				}
			}
		}
		else if (String::endsWith(".rb", outScriptPath))
		{
			scriptType = ScriptType::Ruby;

			auto ruby = Commands::which("ruby");
			if (!ruby.empty())
			{
				shell = std::move(ruby);
				shellFound = true;
			}
		}
		else if (String::endsWith(".pl", outScriptPath))
		{
			scriptType = ScriptType::Perl;

			auto perl = Commands::which("perl");
			if (!perl.empty())
			{
				shell = std::move(perl);
				shellFound = true;
			}
		}
		else if (String::endsWith(".lua", outScriptPath))
		{
			scriptType = ScriptType::Lua;

			auto lua = Commands::which("lua");
			if (!lua.empty())
			{
				shell = std::move(lua);
				shellFound = true;
			}
		}
	}

	if (shellFound)
	{
		// LOG(shell);
		ret.emplace_back(std::move(shell));
	}

	if (!shellFound)
	{
		const bool isPowershellScript = String::endsWith(".ps1", outScriptPath);
#if defined(CHALET_WIN32)
		const bool isBatchScript = String::endsWith(StringList{ ".bat", ".cmd" }, outScriptPath);
		if (isBatchScript || isPowershellScript)
		{
			Path::sanitizeForWindows(outScriptPath);

			auto& powershell = m_tools.powershell();
			auto& cmd = m_tools.commandPrompt();

			if (isBatchScript && !cmd.empty())
			{
				scriptType = ScriptType::WindowsCommand;

				ret.push_back(cmd);
				ret.emplace_back("/c");
			}
			else if (!powershell.empty())
			{
				scriptType = ScriptType::Powershell;

				ret.push_back(powershell);
			}
			else if (isBatchScript)
			{
				Diagnostic::error("{}: The script '{}' requires Command Prompt or Powershell, but they were not found in 'Path'.", m_inputFile, inScript);
				return std::make_pair(StringList{}, scriptType);
			}
			else
			{
				Diagnostic::error("{}: The script '{}' requires powershell, but it was not found in 'Path'.", m_inputFile, inScript);
				return std::make_pair(StringList{}, scriptType);
			}

			shellFound = true;
		}
#else
		if (isPowershellScript)
		{
			auto& powershell = m_tools.powershell();
			if (!powershell.empty())
			{
				scriptType = ScriptType::Powershell;

				ret.push_back(powershell);
			}
			else
			{
				Diagnostic::error("{}: The script '{}' requires powershell open source, but it was not found in 'PATH'.", m_inputFile, inScript);
				return std::make_pair(StringList{}, scriptType);
			}

			shellFound = true;
		}
#endif
	}

	if (!shellFound)
	{
		auto type = shebang;
		if (type.empty())
			type = getScriptTypeString(scriptType);

		if (type.empty())
			Diagnostic::error("{}: The script '{}' was not recognized.", m_inputFile, inScript);
		else
			Diagnostic::error("{}: The script '{}' requires '{}', but it was not found.", m_inputFile, inScript, type);

		return std::make_pair(StringList{}, scriptType);
	}

	ret.emplace_back(std::move(outScriptPath));

	for (const auto& arg : inArguments)
	{
		ret.emplace_back(arg);
	}

#if defined(CHALET_WIN32)
	if (scriptType == ScriptType::Python)
	{
		Environment::set("PYTHONIOENCODING", "utf-8");
		Environment::set("PYTHONLEGACYWINDOWSSTDIO", "utf-8");
	}
#endif

	return std::make_pair(ret, scriptType);
}
}
