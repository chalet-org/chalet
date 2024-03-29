/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/PlatformDependencyManager.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Process/Process.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
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
	if (inValue.empty())
		return;

	if (m_platformRequires.find(inKind) == m_platformRequires.end())
		m_platformRequires.emplace(inKind, StringList{});

	auto& list = m_platformRequires.at(inKind);
	List::addIfDoesNotExist(list, std::move(inValue));
}

/*****************************************************************************/
void PlatformDependencyManager::addRequiredPlatformDependency(const std::string& inKind, StringList&& inValue)
{
	if (inValue.empty())
		return;

	if (m_platformRequires.find(inKind) == m_platformRequires.end())
		m_platformRequires.emplace(inKind, StringList{});

	auto& list = m_platformRequires.at(inKind);
	for (auto&& item : inValue)
	{
		if (item.empty())
			continue;

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

#if defined(CHALET_WIN32)
	auto arch = m_state.info.targetArchitectureString();
#elif defined(CHALET_LINUX)
	auto os = Files::getFileContents("/etc/os-release");
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

	auto arch = m_state.info.targetArchitectureString();
	if (linuxUbuntu || linuxDebian)
	{
		if (m_state.info.targetArchitecture() == Arch::Cpu::X64)
			arch = "amd64";
		else if (m_state.info.targetArchitecture() == Arch::Cpu::ARM64)
			arch = "arm64";
		else if (m_state.info.targetArchitecture() == Arch::Cpu::ARMHF)
			arch = "armhf";
		else if (m_state.info.targetArchitecture() == Arch::Cpu::ARM)
			arch = "arm";
	}
#endif

	StringList errors;

	for (auto& [key, list] : m_platformRequires)
	{
#if defined(CHALET_WIN32)
		if (m_state.environment->isMingw() && String::equals(Keys::ReqWindowsMSYS2, key))
		{
			auto& cc = m_state.toolchain.compilerCxxAny();

			auto detect = String::getPathFolder(cc.path);
			detect = Files::getCanonicalPath(fmt::format("{}/../../usr/bin/msys-2.0.dll", detect));
			if (!Files::pathExists(detect))
				continue; // if toolchain is not MSYS2 (could be some other MinGW version) ... skip

			auto pacman = fmt::format("{}/pacman.exe", String::getPathFolder(detect));
			if (!Files::pathExists(pacman))
			{
				Diagnostic::error("There was a problem detecting the MSYS2 dependencies.");
				return false;
			}

			Timer timer;

			Diagnostic::infoEllipsis("{}MSYS2{}", prefix, suffix);

			StringList cmd{ pacman, "-Q" };
			for (auto& item : list)
			{
				cmd.emplace_back(item);
				cmd.emplace_back(fmt::format("mingw-w64-{}-{}", arch, item));
			}

			auto installed = Process::runOutput(cmd);
			Diagnostic::printDone(timer.asString());

			if (installed.empty())
			{
				Diagnostic::error("There was a problem detecting the MSYS2 dependencies.");
				return false;
			}

			installed = fmt::format("\n{}", installed);

			for (auto& item : list)
			{
				auto findA = fmt::format("\n{} ", item);
				auto findB = fmt::format("\nmingw-w64-{}-{} ", arch, item);

				bool existsA = String::contains(findA, installed);
				bool existsB = String::contains(findB, installed);

				bool exists = existsB || existsA;

				if (existsB)
					Diagnostic::subInfoEllipsis("mingw-w64-{}-{}", arch, item);
				else
					Diagnostic::subInfoEllipsis("{}", item);

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
			if (m_state.info.targetArchitecture() != m_state.info.hostArchitecture())
			{
				Diagnostic::error("Homebrew was required by the build, but can only be used with the host architecture: ({})", m_state.info.hostArchitectureString());
				return false;
			}

			auto brew = Files::which("brew");
			if (brew.empty() || !Files::pathExists("/usr/local/Cellar"))
			{
				Diagnostic::error("Homebrew was required by the build, but was not detected.");
				return false;
			}

			// Homebrew invocation is slow, so we'll just detect the Cellar paths

			Diagnostic::info("{}Homebrew{}", prefix, suffix);
			for (auto& item : list)
			{
				Diagnostic::subInfoEllipsis("{}", item);

				auto path = fmt::format("/usr/local/Cellar/{}", item);
				bool exists = Files::pathExists(path) && Files::pathIsDirectory(path);
				Diagnostic::printFound(exists);

				if (!exists)
				{
					errors.push_back(fmt::format("Homebrew dependency '{}' was not found.", item));
				}
			}
		}
		else if (String::equals(Keys::ReqMacOSMacPorts, key))
		{
			if (m_state.info.targetArchitecture() != m_state.info.hostArchitecture())
			{
				Diagnostic::error("MacPorts was required by the build, but can only be used with the host architecture: ({})", m_state.info.hostArchitectureString());
				return false;
			}

			auto port = Files::which("port");
			if (port.empty())
			{
				Diagnostic::error("MacPorts was required by the build, but was not detected.");
				return false;
			}

			Timer timer;
			Diagnostic::infoEllipsis("{}MacPorts{}", prefix, suffix);
			auto installed = Process::runOutput({ port, "installed" });
			Diagnostic::printDone(timer.asString());

			if (installed.empty())
			{
				Diagnostic::error("There was a problem detecting the MacPorts dependencies.");
				return false;
			}

			for (auto& item : list)
			{
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
		// TODO: MSYS2 cross-compile from linux

		if ((linuxArch && String::equals(Keys::ReqArchLinuxSystem, key))
			|| (linuxManjaro && String::equals(Keys::ReqManjaroSystem, key)))
		{
			auto pacman = Files::which("pacman");
			if (!Files::pathExists(pacman))
			{
				Diagnostic::error("There was a problem detecting the system dependencies.");
				return false;
			}

			Timer timer;

			Diagnostic::infoEllipsis("{}system{}", prefix, suffix);

			StringList cmd{ pacman, "-Q" };
			for (auto& item : list)
				cmd.emplace_back(item);

			auto installed = Process::runOutput(cmd);
			Diagnostic::printDone(timer.asString());

			if (installed.empty())
			{
				Diagnostic::error("There was a problem detecting the system dependencies.");
				return false;
			}

			installed = fmt::format("\n{}", installed);

			for (auto& item : list)
			{
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
			auto apt = Files::which("apt");
			if (!Files::pathExists(apt))
			{
				Diagnostic::error("There was a problem detecting the system dependencies.");
				return false;
			}

			Timer timer;

			Diagnostic::infoEllipsis("{}system{}", prefix, suffix);

			StringList cmd{ apt, "list", "--installed" };
			for (auto& item : list)
				cmd.emplace_back(item);

			auto installed = Process::runOutput(cmd);
			Diagnostic::printDone(timer.asString());

			if (installed.empty())
			{
				Diagnostic::error("There was a problem detecting the system dependencies.");
				return false;
			}

			auto split = String::split(installed, '\n');

			StringList foundItems;

			for (auto& line : split)
			{
				if (line.empty())
					continue;

				for (auto& item : list)
				{
					if (String::startsWith(fmt::format("{}/", item), line)
						&& (String::contains(fmt::format(" {} [", arch), line) || String::contains(" all [", line)))
						foundItems.emplace_back(item);
				}
			}

			for (auto& item : list)
			{
				Diagnostic::subInfoEllipsis("{}", item);

				bool exists = List::contains(foundItems, item);
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
			auto yum = Files::which("yum");
			if (!Files::pathExists(yum))
			{
				Diagnostic::error("There was a problem detecting the system dependencies.");
				return false;
			}

			Timer timer;

			Diagnostic::infoEllipsis("{}system{}", prefix, suffix);

			StringList cmd{ yum, "list", "--installed", "--color=off" };
			for (auto& item : list)
				cmd.emplace_back(item);

			auto installed = Process::runOutput(cmd);
			Diagnostic::printDone(timer.asString());

			if (installed.empty())
			{
				Diagnostic::error("There was a problem detecting the system dependencies.");
				return false;
			}

			for (auto& item : list)
			{
				Diagnostic::subInfoEllipsis("{}", item);

				auto findA = fmt::format("\n{}.{}", item, arch);
				auto findB = fmt::format("\n{}.noarch", item);

				bool exists = String::contains(findA, installed) || String::contains(findB, installed);
				Diagnostic::printFound(exists);

				if (!exists)
				{
					errors.push_back(fmt::format("system dependency '{}' was not found.", item));
				}
			}
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
