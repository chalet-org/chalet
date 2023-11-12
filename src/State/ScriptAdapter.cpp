/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/ScriptAdapter.hpp"

#include "State/AncillaryTools.hpp"
#include "Terminal/Commands.hpp"
#include "Process/Environment.hpp"
#include "Utility/Path.hpp"
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
		case ScriptType::Tcl:
			return "tclsh";
		case ScriptType::Awk:
			return "awk";
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
	if (String::startsWith("python", lower) && String::equals({ "python", "python3", "python2" }, lower))
		return ScriptType::Python;

	if (String::equals("lua", lower))
		return ScriptType::Lua;

	if (String::equals("ruby", lower))
		return ScriptType::Ruby;

	if (String::equals("perl", lower))
		return ScriptType::Perl;

	if (String::equals("tclsh", lower))
		return ScriptType::Tcl;

	if (String::equals("awk", lower))
		return ScriptType::Awk;

	if (String::equals("pwsh", lower))
		return ScriptType::Powershell;

	return ScriptType::UnixShell;
}
}

/*****************************************************************************/
ScriptAdapter::ScriptAdapter(const AncillaryTools& inTools) :
	m_tools(inTools),
	m_executables({
		{ ScriptType::None, std::string() },
	})
{
}

/*****************************************************************************/
std::pair<std::string, ScriptType> ScriptAdapter::getScriptTypeFromPath(const std::string& inScript, const std::string& inInputFile) const
{
#if defined(CHALET_WIN32)
	std::string parsedScriptPath = inScript;
	if (String::endsWith(".exe", parsedScriptPath))
		parsedScriptPath = parsedScriptPath.substr(0, parsedScriptPath.size() - 4);

	auto outScriptPath = Commands::which(parsedScriptPath);

	auto gitPath = AncillaryTools::getPathToGit();
	if (!gitPath.empty())
	{
		auto rootPath = String::getPathFolder(String::getPathFolder(gitPath));
		gitPath = fmt::format("{}/usr/bin", rootPath);

		if (!Commands::pathExists(gitPath))
		{
			gitPath.clear();
		}
	}
#else
	auto outScriptPath = Commands::which(inScript);
	std::string gitPath;
#endif
	if (outScriptPath.empty())
		outScriptPath = Commands::getAbsolutePath(inScript);

	if (!Commands::pathExists(outScriptPath))
	{
		Diagnostic::error("{}: The script '{}' was not found. Aborting.", inInputFile, inScript);
		return std::make_pair(std::string(), ScriptType::None);
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
				shebang = shebang.substr(space + 1);
				scriptType = getScriptTypeFromString(shebang);
				shell = Commands::which(shebang);
				shellFound = !shell.empty();

				if (!shellFound && String::startsWith("python", shebang))
				{
					// Handle python 2/3 nastiness
					// This is mostly for convenience across platforms
					// For instance, mac uses "python3" and removed "python"
					// while Windows installers use only "python.exe" now
					//
					if (String::equals("python", shebang))
					{
						shell = Commands::which("python3");
					}
					else if (String::equals("python3", shebang))
					{
						shell = Commands::which("python");
					}
					else if (String::equals("python2", shebang)) // just in case
					{
						shell = Commands::which("python");
					}
				}
				else if (!shellFound && !gitPath.empty() && String::equals("perl", shebang))
				{
					shell = fmt::format("{}/perl.exe", gitPath);
					shellFound = Commands::pathExists(shell);
				}
				else if (!shellFound && !gitPath.empty() && String::equals("awk", shebang))
				{
					shell = fmt::format("{}/awk.exe", gitPath);
					shellFound = Commands::pathExists(shell);
				}
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
				else
				{
					// just in case
					python = Commands::which("python2");
					if (!python.empty())
					{
						shell = std::move(python);
						shellFound = true;
					}
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
			else
			{
				if (!gitPath.empty())
				{
					perl = fmt::format("{}/perl.exe", gitPath);
					if (Commands::pathExists(perl))
					{
						shell = std::move(perl);
						shellFound = true;
					}
				}
			}
		}
		else if (String::endsWith(".tcl", outScriptPath))
		{
			scriptType = ScriptType::Tcl;

			auto perl = Commands::which("tclsh");
			if (!perl.empty())
			{
				shell = std::move(perl);
				shellFound = true;
			}
		}
		else if (String::endsWith(".awk", outScriptPath))
		{
			scriptType = ScriptType::Awk;

			auto perl = Commands::which("awk");
			if (!perl.empty())
			{
				shell = std::move(perl);
				shellFound = true;
			}
			else
			{
				if (!gitPath.empty())
				{
					perl = fmt::format("{}/awk.exe", gitPath);
					if (Commands::pathExists(perl))
					{
						shell = std::move(perl);
						shellFound = true;
					}
				}
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

	if (!shellFound)
	{
		const bool isPowershellScript = String::endsWith(".ps1", outScriptPath);
#if defined(CHALET_WIN32)
		const bool isBatchScript = String::endsWith(StringList{ ".bat", ".cmd" }, outScriptPath);
		if (isBatchScript || isPowershellScript)
		{
			Path::windows(outScriptPath);

			const auto& powershell = m_tools.powershell();
			const auto& cmd = m_tools.commandPrompt();

			if (isBatchScript && !cmd.empty())
			{
				scriptType = ScriptType::WindowsCommand;
				shell = cmd;
			}
			else if (!powershell.empty())
			{
				scriptType = ScriptType::Powershell;
				shell = powershell;
			}
			else if (isBatchScript)
			{
				Diagnostic::error("{}: The script '{}' requires Command Prompt or Powershell, but they were not found in 'Path'.", inInputFile, inScript);
				return std::make_pair(std::string(), ScriptType::None);
			}
			else
			{
				Diagnostic::error("{}: The script '{}' requires powershell, but it was not found in 'Path'.", inInputFile, inScript);
				return std::make_pair(std::string(), ScriptType::None);
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
				shell = powershell;
			}
			else
			{
				Diagnostic::error("{}: The script '{}' requires powershell open source, but it was not found in 'PATH'.", inInputFile, inScript);
				return std::make_pair(std::string(), ScriptType::None);
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
			Diagnostic::error("{}: The script '{}' was not recognized.", inInputFile, inScript);
		else
			Diagnostic::error("{}: The script '{}' requires '{}', but it was not found.", inInputFile, inScript, type);

		return std::make_pair(std::string(), ScriptType::None);
	}

	// LOG(shell);
	chalet_assert(scriptType != ScriptType::None, "bad script type");

	m_executables[scriptType] = std::move(shell);
	return std::make_pair(std::move(outScriptPath), scriptType);
}

/*****************************************************************************/
const std::string& ScriptAdapter::getExecutable(const ScriptType inType) const
{
	if (m_executables.find(inType) != m_executables.end())
		return m_executables.at(inType);

	return m_executables.at(ScriptType::None);
}
}
