/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/ScriptRunner.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Libraries/Format.hpp"
#include "State/CacheTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Subprocess.hpp"

namespace chalet
{
/*****************************************************************************/
ScriptRunner::ScriptRunner(const CacheTools& inTools, const std::string& inBuildFile, const bool inCleanOutput) :
	m_tools(inTools),
	m_buildFile(inBuildFile),
	m_cleanOutput(inCleanOutput)
{
}

/*****************************************************************************/
bool ScriptRunner::run(const StringList& inScripts)
{
	for (auto& scriptPath : inScripts)
	{
		std::ptrdiff_t i = &scriptPath - &inScripts.front();
		if (i == 0)
		{
			std::cout << Output::getAnsiReset() << std::flush;
		}

		if (!run(scriptPath))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool ScriptRunner::run(const std::string& inScript)
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
		Diagnostic::error(fmt::format("{}: The script '{}' was not found. Aborting.", m_buildFile, inScript));
		return false;
	}

	Commands::setExecutableFlag(outScriptPath, m_cleanOutput);

	StringList command;

	std::string shebang;
	bool shellFound = false;
	if (m_tools.bashAvailable())
	{
		std::string shell;
		shebang = Commands::readShebangFromFile(outScriptPath);
		if (!shebang.empty())
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
				Diagnostic::error(fmt::format("{}: The script '{}' requires Command Prompt or Powershell, but they were not found in 'Path'.", m_buildFile, inScript));
				return false;
			}
			else
			{
				Diagnostic::error(fmt::format("{}: The script '{}' requires powershell, but it was not found in 'Path'.", m_buildFile, inScript));
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
				Diagnostic::error(fmt::format("{}: The script '{}' requires powershell open source, but it was not found in 'PATH'.", m_buildFile, inScript));
				return false;
			}

			shellFound = true;
		}
#endif
	}

	if (!shellFound)
	{
		if (shebang.empty())
			Diagnostic::error(fmt::format("{}: The script '{}' was not recognized.", m_buildFile, inScript));
		else
			Diagnostic::error(fmt::format("{}: The script '{}' requires the shell '{}', but it was not found.", m_buildFile, inScript, shebang));
		return false;
	}

	command.push_back(std::move(outScriptPath));

	if (!Commands::subprocess(command, m_cleanOutput))
	{
		Output::lineBreak();
		auto exitCode = Subprocess::getLastExitCode();
		Diagnostic::error(fmt::format("{}: The script '{}' exited with code {}.", m_buildFile, inScript, exitCode));
		return false;
	}

	return true;
}
}
