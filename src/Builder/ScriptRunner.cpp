/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/ScriptRunner.hpp"

#include "Core/CommandLineInputs.hpp"

#include "State/AncillaryTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Subprocess.hpp"

namespace chalet
{
/*****************************************************************************/
ScriptRunner::ScriptRunner(const AncillaryTools& inTools, const std::string& inBuildFile) :
	m_tools(inTools),
	m_buildFile(inBuildFile)
{
}

/*****************************************************************************/
bool ScriptRunner::run(const StringList& inScripts, const bool inShowExitCode)
{
	for (auto& scriptPath : inScripts)
	{
		std::ptrdiff_t i = &scriptPath - &inScripts.front();
		if (i == 0)
		{
			std::cout << Output::getAnsiReset() << std::flush;
		}

		if (!run(scriptPath, inShowExitCode))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool ScriptRunner::run(const std::string& inScript, const bool inShowExitCode)
{
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
		Diagnostic::error("{}: The script '{}' was not found. Aborting.", m_buildFile, inScript);
		return false;
	}

	Commands::setExecutableFlag(outScriptPath);

	StringList command;

	std::string shebang;
	bool shellFound = false;
	if (m_tools.bashAvailable())
	{
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
					shell = Commands::which(shebang.substr(space + 1));
					shellFound = !shell.empty();
				}
			}
			else
			{
				shell = shebang;
				shellFound = Commands::pathExists(shell);

				if (!shellFound)
				{
					if (String::startsWith("/bin", shell))
					{
						shell = fmt::format("/usr{}", shell);
						shellFound = Commands::pathExists(shell);

						if (!shellFound)
						{
							shell = fmt::format("/usr/local{}", shebang);
							shellFound = Commands::pathExists(shell);
						}
					}
					else
					{
						shell = Environment::getShell();
						shellFound = !shell.empty();
					}
				}
			}
		}
		if (!shellFound)
		{
			if (String::endsWith(".sh", outScriptPath))
			{
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
				if (!m_tools.python3().empty())
				{
					shell = m_tools.python3();
					shellFound = true;
				}
				else if (!m_tools.python().empty())
				{
					shell = m_tools.python();
					shellFound = true;
				}
			}
			else if (String::endsWith(".rb", outScriptPath))
			{
				if (!m_tools.ruby().empty())
				{
					shell = m_tools.ruby();
					shellFound = true;
				}
			}
			else if (String::endsWith(".pl", outScriptPath))
			{
				if (!m_tools.perl().empty())
				{
					shell = m_tools.perl();
					shellFound = true;
				}
			}
			else if (String::endsWith(".lua", outScriptPath))
			{
				if (!m_tools.lua().empty())
				{
					shell = m_tools.lua();
					shellFound = true;
				}
			}
		}

		if (shellFound)
		{
			// LOG(shell);
			command.push_back(std::move(shell));
		}
	}

	if (!shellFound)
	{
		const bool isPowershellScript = String::endsWith(".ps1", outScriptPath);
#if defined(CHALET_WIN32)
		const bool isBatchScript = String::endsWith({ ".bat", ".cmd" }, outScriptPath);
		if (isBatchScript || isPowershellScript)
		{
			Path::sanitizeForWindows(outScriptPath);

			auto& powershell = m_tools.powershell();
			auto& cmd = m_tools.commandPrompt();

			if (isBatchScript && !cmd.empty())
			{
				command.push_back(cmd);
				command.push_back("/c");
			}
			else if (!powershell.empty())
			{
				command.push_back(powershell);
			}
			else if (isBatchScript)
			{
				Diagnostic::error("{}: The script '{}' requires Command Prompt or Powershell, but they were not found in 'Path'.", m_buildFile, inScript);
				return false;
			}
			else
			{
				Diagnostic::error("{}: The script '{}' requires powershell, but it was not found in 'Path'.", m_buildFile, inScript);
				return false;
			}

			shellFound = true;
		}
#else
		if (isPowershellScript)
		{
			auto& powershell = m_tools.powershell();
			if (!powershell.empty())
			{
				command.push_back(powershell);
			}
			else
			{
				Diagnostic::error("{}: The script '{}' requires powershell open source, but it was not found in 'PATH'.", m_buildFile, inScript);
				return false;
			}

			shellFound = true;
		}
#endif
	}

	if (!shellFound)
	{
		if (shebang.empty())
			Diagnostic::error("{}: The script '{}' was not recognized.", m_buildFile, inScript);
		else
			Diagnostic::error("{}: The script '{}' requires the shell '{}', but it was not found.", m_buildFile, inScript, shebang);
		return false;
	}

	command.push_back(std::move(outScriptPath));

	bool result = Commands::subprocess(command);
	auto exitCode = Subprocess::getLastExitCode();
	auto message = fmt::format("{} exited with code: {}", inScript, exitCode);

	if (inShowExitCode || !result)
	{
		Output::lineBreak();
		Output::print(result ? Color::Reset : Color::Red, message, true);

		if (!inShowExitCode && !result)
			Output::lineBreak();
	}

	return result;
}
}
