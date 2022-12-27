/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/PlatformDependencyManager.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
PlatformDependencyManager::PlatformDependencyManager(const BuildState& inState) :
	m_state(inState)
{
	UNUSED(m_state);
}

/*****************************************************************************/
void PlatformDependencyManager::addRequiredPlatformDependency(const std::string& inKind, std::string&& inValue)
{
	if (m_platformRequires.find(inKind) == m_platformRequires.end())
		m_platformRequires.emplace(inKind, StringList{});

	auto& list = m_platformRequires.at(inKind);
	List::addIfDoesNotExist(list, std::move(inValue));
}

/*****************************************************************************/
void PlatformDependencyManager::addRequiredPlatformDependency(const std::string& inKind, StringList&& inValue)
{
	if (m_platformRequires.find(inKind) == m_platformRequires.end())
		m_platformRequires.emplace(inKind, StringList{});

	auto& list = m_platformRequires.at(inKind);
	for (auto&& item : inValue)
	{
		List::addIfDoesNotExist(list, std::move(item));
	}
}

/*****************************************************************************/
bool PlatformDependencyManager::hasRequired()
{
	if (m_platformRequires.empty())
		return true;

	static const char prefix[] = "Verifying required ";
	static const char suffix[] = " packages";

#if defined(CHALET_LINUX)
	auto os = Commands::getFileContents("/etc/os-release");
	if (os.empty())
	{
		Diagnostic::error("There was a problem detecting the Linux OS ID.");
		return false;
	}

	auto start = os.find("\nID=");
	auto end = start != std::string::npos ? os.find('\n', start + 4) : std::string::npos;
	if (start == std::string::npos || end == std::string::npos)
	{
		Diagnostic::error("There was a problem detecting the Linux OS ID.");
		return false;
	}
	start += 4;
	auto id = os.substr(start, end - start);
	if (id.empty())
	{
		Diagnostic::error("There was a problem detecting the Linux OS ID.");
		return false;
	}

	// decent collection of os-release files:
	//   https://github.com/zyga/os-release-zoo
	//
	const bool linuxArch = String::equals("arch", id);
	const bool linuxManjaro = String::equals("manjaro", id);
	const bool linuxUbuntu = String::equals("ubuntu", id);
	const bool linuxDebian = String::equals("debian", id);
	const bool linuxFedora = String::equals("fedora", id);
	const bool linuxRedHat = String::equals("rhel", id);
#endif

	StringList errors;

	for (auto& [key, list] : m_platformRequires)
	{
#if defined(CHALET_WIN32)
		if (m_state.environment->isMingw() && String::equals(Keys::ReqWindowsMSYS2, key))
		{
			auto& cc = m_state.toolchain.compilerCxxAny();

			auto detect = String::getPathFolder(cc.path);
			detect = Commands::getCanonicalPath(fmt::format("{}/../../usr/bin/msys-2.0.dll", detect));
			if (!Commands::pathExists(detect))
				continue; // if toolchain is not MSYS2 (could be some other MinGW version) ... skip

			auto pacman = fmt::format("{}/pacman.exe", String::getPathFolder(detect));
			if (!Commands::pathExists(pacman))
			{
				Diagnostic::error("There was a problem detecting the MSYS2 dependencies.");
				return false;
			}

			Timer timer;

			Diagnostic::infoEllipsis("{}MSYS2{}", prefix, suffix);
			auto query = String::join(list);
			if (query.empty())
				continue;

			StringList cmd{ pacman, "-Q" };
			for (auto& item : list)
				cmd.emplace_back(item);

			auto installed = Commands::subprocessOutput(cmd);
			Diagnostic::printDone(timer.asString());

			if (installed.empty())
			{
				Diagnostic::error("There was a problem detecting the MSYS2 dependencies.");
				return false;
			}

			installed = fmt::format("\n{}", installed);

			for (auto& item : list)
			{
				if (item.empty())
					continue;

				Diagnostic::subInfoEllipsis("{}", item);

				auto find = fmt::format("\n{} ", item);

				bool exists = String::contains(find, installed);
				Diagnostic::printFound(exists);

				if (!exists)
				{
					errors.push_back(fmt::format("MSYS2 dependency '{}' was not found.", item));
				}
			}
		}
#elif defined(CHALET_MACOS)
		if (String::equals(Keys::ReqMacOSHomebrew, key))
		{
			// TODO: detect homebrew

			Diagnostic::info("{}Homebrew{}", prefix, suffix);
			for (auto& item : list)
			{
				if (item.empty())
					continue;

				Diagnostic::subInfoEllipsis("{}", item);

				auto path = fmt::format("/usr/local/Cellar/{}", item);
				bool exists = Commands::pathExists(path) && Commands::pathIsDirectory(path);
				Diagnostic::printFound(exists);

				if (!exists)
				{
					errors.push_back(fmt::format("Homebrew dependency '{}' was not found.", item));
				}
			}
		}
		else if (String::equals(Keys::ReqMacOSMacPorts, key))
		{

			auto port = Commands::which("port");
			if (port.empty())
			{
				Diagnostic::error("MacPorts was required by the build, but could not be detected.");
				return false;
			}

			Timer timer;
			Diagnostic::infoEllipsis("{}MacPorts{}", prefix, suffix);
			auto installed = Commands::subprocessOutput({ port, "installed" });
			Diagnostic::printDone(timer.asString());

			if (installed.empty())
			{
				Diagnostic::error("There was a problem detecting the MacPorts dependencies.");
				return false;
			}

			for (auto& item : list)
			{
				if (item.empty())
					continue;

				Diagnostic::subInfoEllipsis("{}", item);

				auto find = fmt::format("\n  {} ", item);

				bool exists = String::contains(find, installed);
				Diagnostic::printFound(exists);

				if (!exists)
				{
					errors.push_back(fmt::format("MacPorts dependency '{}' was not found.", item));
				}
			}
		}
#else
		if ((linuxArch && String::equals(Keys::ReqArchLinuxSystem, key))
			|| (linuxManjaro && String::equals(Keys::ReqManjaroSystem, key)))
		{
			auto pacman = Commands::which("pacman");
			if (!Commands::pathExists(pacman))
			{
				Diagnostic::error("There was a problem detecting the system dependencies.");
				return false;
			}

			Timer timer;

			Diagnostic::infoEllipsis("{}system{}", prefix, suffix);
			auto query = String::join(list);
			if (query.empty())
				continue;

			StringList cmd{ pacman, "-Q" };
			for (auto& item : list)
				cmd.emplace_back(item);

			auto installed = Commands::subprocessOutput(cmd);
			Diagnostic::printDone(timer.asString());

			if (installed.empty())
			{
				Diagnostic::error("There was a problem detecting the system dependencies.");
				return false;
			}

			installed = fmt::format("\n{}", installed);

			for (auto& item : list)
			{
				if (item.empty())
					continue;

				Diagnostic::subInfoEllipsis("{}", item);

				auto find = fmt::format("\n{} ", item);

				bool exists = String::contains(find, installed);
				Diagnostic::printFound(exists);

				if (!exists)
				{
					errors.push_back(fmt::format("system dependency '{}' was not found.", item));
				}
			}
		}
		else if ((linuxUbuntu && String::equals(Keys::ReqUbuntuSystem, key))
			|| (linuxDebian && String::equals(Keys::ReqDebianSystem, key)))
		{
			auto apt = Commands::which("apt");
			if (!Commands::pathExists(apt))
			{
				Diagnostic::error("There was a problem detecting the system dependencies.");
				return false;
			}

			Timer timer;

			Diagnostic::infoEllipsis("{}system{}", prefix, suffix);
			auto query = String::join(list);
			if (query.empty())
				continue;

			StringList cmd{ apt, "list", "--installed" };
			for (auto& item : list)
				cmd.emplace_back(item);

			auto installed = Commands::subprocessOutput(cmd);
			Diagnostic::printDone(timer.asString());

			if (installed.empty())
			{
				Diagnostic::error("There was a problem detecting the system dependencies.");
				return false;
			}

			for (auto& item : list)
			{
				if (item.empty())
					continue;

				Diagnostic::subInfoEllipsis("{}", item);

				auto find = fmt::format("\n{}/", item);

				bool exists = String::contains(find, installed);
				Diagnostic::printFound(exists);

				if (!exists)
				{
					errors.push_back(fmt::format("system dependency '{}' was not found.", item));
				}
			}
		}
		else if ((linuxFedora && String::equals(Keys::ReqFedoraSystem, key))
			|| (linuxRedHat && String::equals(Keys::ReqRedHatSystem, key)))
		{
		}
#endif
	}

	if (!errors.empty())
	{
		// for (auto it = errors.rbegin(); it != errors.rend(); ++it)
		// {
		// 	Diagnostic::error(*it);
		// }
		Diagnostic::error("One or more required platform dependencies were not found.");
		return false;
	}

	m_platformRequires.clear();

	return true;
}
}
