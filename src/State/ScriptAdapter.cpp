/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/ScriptAdapter.hpp"

#include "Process/Environment.hpp"
#include "State/AncillaryTools.hpp"
#include "System/Files.hpp"
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
ScriptAdapter::PathResult ScriptAdapter::getScriptTypeFromPath(const std::string& inScript, const std::string& inInputFile) const
{
	PathResult ret;
#if defined(CHALET_WIN32)
	auto exe = Files::getPlatformExecutableExtension();
	std::string parsedScriptPath = inScript;
	if (String::endsWith(exe, parsedScriptPath))
		parsedScriptPath = parsedScriptPath.substr(0, parsedScriptPath.size() - 4);

	auto outScriptPath = Files::which(parsedScriptPath);

	auto gitPath = AncillaryTools::getPathToGit();
	std::string gitRootPath;
	if (!gitPath.empty())
	{
		gitRootPath = String::getPathFolder(String::getPathFolder(gitPath));
		gitPath = fmt::format("{}/usr/bin", gitRootPath);

		if (!Files::pathExists(gitPath))
		{
			gitPath.clear();
		}
	}
#else
	auto outScriptPath = Files::which(inScript);
	std::string gitPath;
#endif
	if (outScriptPath.empty())
		outScriptPath = Files::getAbsolutePath(inScript);

	if (!Files::pathExists(outScriptPath))
	{
		Diagnostic::error("{}: The script '{}' was not found. Aborting.", inInputFile, inScript);
		return ret;
	}

	Files::setExecutableFlag(outScriptPath);

	std::string shebang;
	bool shellFound = false;
	ScriptType scriptType = ScriptType::None;

	std::string shell;
	shebang = readShebangFromFile(outScriptPath);
	if (!shebang.empty())
	{
		if (String::contains("/env ", shebang))
		{
			auto envPart = shebang.find("/env ");
			auto search = shebang.substr(0, envPart + 4);
			bool envExists = String::equals("/usr/bin/env", search);
			if (!envExists)
			{
#if defined(CHALET_WIN32)
				std::string oldSearch = search;
				if (String::startsWith('/', search) && !gitRootPath.empty())
				{
					search = fmt::format("{}{}.exe", gitRootPath, search);
					envExists = Files::pathExists(search);
				}
				else
#endif
				{
					envExists = Files::pathExists(search);
				}

				if (!envExists)
				{
					Diagnostic::error("Did you mean to use '#!/usr/bin/env'?");
#if defined(CHALET_WIN32)
					Diagnostic::error("{}: The script requires '{}' ({}), but it does not exist. Aborting.", inInputFile, oldSearch, search);
#else
					Diagnostic::error("{}: The script requires '{}', but it does not exist. Aborting.", inInputFile, search);
#endif
					return ret;
				}
			}

			if (!search.empty())
			{
				shebang = shebang.substr(envPart + 5);
				scriptType = getScriptTypeFromString(shebang);

				if (String::equals("bash", shebang))
					shell = m_tools.bash();
				else
					shell = Files::which(shebang);

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
						shell = Files::which("python3");
					}
					else if (String::equals("python3", shebang))
					{
						shell = Files::which("python");
					}
					else if (String::equals("python2", shebang)) // just in case
					{
						shell = Files::which("python");
					}
				}
				else if (!shellFound && !gitPath.empty() && String::equals("perl", shebang))
				{
					shell = fmt::format("{}/perl.exe", gitPath);
					shellFound = Files::pathExists(shell);
				}
				else if (!shellFound && !gitPath.empty() && String::equals("awk", shebang))
				{
					shell = fmt::format("{}/awk.exe", gitPath);
					shellFound = Files::pathExists(shell);
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
				shellFound = Files::pathExists(shell);

				if (!shellFound)
				{
					shell = Files::which(search);
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

			auto python = Files::which("python3");
			if (!python.empty())
			{
				shell = std::move(python);
				shellFound = true;
			}
			else
			{
				python = Files::which("python");
				if (!python.empty())
				{
					shell = std::move(python);
					shellFound = true;
				}
				else
				{
					// just in case
					python = Files::which("python2");
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

			auto ruby = Files::which("ruby");
			if (!ruby.empty())
			{
				shell = std::move(ruby);
				shellFound = true;
			}
		}
		else if (String::endsWith(".pl", outScriptPath))
		{
			scriptType = ScriptType::Perl;

			auto perl = Files::which("perl");
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
					if (Files::pathExists(perl))
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

			auto perl = Files::which("tclsh");
			if (!perl.empty())
			{
				shell = std::move(perl);
				shellFound = true;
			}
		}
		else if (String::endsWith(".awk", outScriptPath))
		{
			scriptType = ScriptType::Awk;

			auto perl = Files::which("awk");
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
					if (Files::pathExists(perl))
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

			auto lua = Files::which("lua");
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
			Path::toWindows(outScriptPath);

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
				return ret;
			}
			else
			{
				Diagnostic::error("{}: The script '{}' requires powershell, but it was not found in 'Path'.", inInputFile, inScript);
				return ret;
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
				return ret;
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

		return ret;
	}

	// LOG(shell);
	chalet_assert(scriptType != ScriptType::None, "bad script type");

	m_executables[scriptType] = std::move(shell);
	ret.file = std::move(outScriptPath);
	ret.type = scriptType;
	return ret;
}

/*****************************************************************************/
const std::string& ScriptAdapter::getExecutable(const ScriptType inType) const
{
	if (m_executables.find(inType) != m_executables.end())
		return m_executables.at(inType);

	return m_executables.at(ScriptType::None);
}

/*****************************************************************************/
std::string ScriptAdapter::readShebangFromFile(const std::string& inFile) const
{
	std::string ret;

	if (Files::pathExists(inFile))
	{
		std::ifstream file{ inFile };
		std::getline(file, ret); // get the first line in the file

		if (String::startsWith("#!", ret))
		{
			ret = ret.substr(2);

			if (String::contains("/env ", ret))
			{
				// we'll parse the rest later
			}
			else
			{
				auto space = ret.find_first_of(" ");
				if (space != std::string::npos)
					ret = std::string();
			}
		}
		else
			ret = std::string();
	}

	return ret;
}
}
