/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/PlatformDependencyManager.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Process/Process.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
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
}

/*****************************************************************************/
void PlatformDependencyManager::addRequiredDependency(const std::string& inKind, std::string&& inValue)
{
	if (inValue.empty())
		return;

	auto supportedSystem = getSupportedSystemFromString(inKind);
	if (isSupportedSystemValid(supportedSystem))
	{
		if (m_platformRequires.find(supportedSystem) == m_platformRequires.end())
			m_platformRequires.emplace(supportedSystem, StringList{});

		auto& list = m_platformRequires.at(supportedSystem);
		List::addIfDoesNotExist(list, std::move(inValue));
	}
}

/*****************************************************************************/
void PlatformDependencyManager::addRequiredDependency(const std::string& inKind, StringList&& inValue)
{
	if (inValue.empty())
		return;

	auto supportedSystem = getSupportedSystemFromString(inKind);
	if (isSupportedSystemValid(supportedSystem))
	{
		if (m_platformRequires.find(supportedSystem) == m_platformRequires.end())
			m_platformRequires.emplace(supportedSystem, StringList{});

		auto& list = m_platformRequires.at(supportedSystem);
		for (auto&& item : inValue)
		{
			if (item.empty())
				continue;

			List::addIfDoesNotExist(list, std::move(item));
		}
	}
}

/*****************************************************************************/
bool PlatformDependencyManager::initialize()
{
#if defined(CHALET_WIN32)
	if (m_state.environment->isMingw())
	{
		m_system = SupportedSystem::WindowsMSYS2;
		m_systemString = "msys2";
	}
	else
	{
		m_system = SupportedSystem::Windows;
		m_systemString = "windows";
	}

	m_archString = m_state.info.targetArchitectureString();
#elif defined(CHALET_MACOS)
	m_system = SupportedSystem::MacOS;
	m_archString = m_state.info.targetArchitectureString();
#elif defined(CHALET_LINUX)
	m_system = SupportedSystem::Unknown;

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
	m_systemString = os.substr(start, end - start);
	if (m_systemString.empty())
	{
		Diagnostic::error("There was a problem detecting the Linux OS ID.");
		return false;
	}

	// decent collection of os-release files:
	//   https://github.com/zyga/os-release-zoo
	//
	if (String::equals("arch", m_systemString))
		m_system = SupportedSystem::LinuxArch;
	else if (String::equals("manjaro", m_systemString))
		m_system = SupportedSystem::LinuxManjaro;
	else if (String::equals("ubuntu", m_systemString))
		m_system = SupportedSystem::LinuxUbuntu;
	else if (String::equals("debian", m_systemString))
		m_system = SupportedSystem::LinuxDebian;
	else if (String::equals("fedora", m_systemString))
		m_system = SupportedSystem::LinuxFedora;
	else if (String::equals("rhel", m_systemString))
		m_system = SupportedSystem::LinuxRedHat;

	m_archString = m_state.info.targetArchitectureString();
	if (m_system == SupportedSystem::LinuxDebian || m_system == SupportedSystem::LinuxUbuntu)
	{
		if (m_state.info.targetArchitecture() == Arch::Cpu::X64)
			m_archString = "amd64";
		else if (m_state.info.targetArchitecture() == Arch::Cpu::ARM64)
			m_archString = "arm64";
		else if (m_state.info.targetArchitecture() == Arch::Cpu::ARMHF)
			m_archString = "armhf";
		else if (m_state.info.targetArchitecture() == Arch::Cpu::ARM)
			m_archString = "arm";
	}
#endif

	return true;
}

/*****************************************************************************/
bool PlatformDependencyManager::validate()
{
	if (m_platformRequires.empty())
		return true;

	if (m_systemString.empty() || m_archString.empty() || m_system == SupportedSystem::Unknown)
	{
		Diagnostic::error("There was a problem validating the platform dependencies.");
		return false;
	}

	StringList errors;

#if defined(CHALET_WIN32)
	if (m_state.environment->isMingw())
	{
		auto& list = m_platformRequires.at(SupportedSystem::WindowsMSYS2);
		if (!checkDependenciesWithPacmanMSYS2(list, errors))
			return false;
	}
#elif defined(CHALET_MACOS)
	for (auto& [supportedSystem, list] : m_platformRequires)
	{
		if (supportedSystem == SupportedSystem::MacOSHomebrew)
		{
			if (!checkDependenciesWithHomebrew(list, errors))
				return false;
		}
		else if (supportedSystem == SupportedSystem::MacOSMacPorts)
		{
			if (!checkDependenciesWithMacPorts(list, errors))
				return false;
		}
	}
#else
	// m_platformRequires should have m_system if we got here, but for safety's sake
	if (m_platformRequires.find(m_system) != m_platformRequires.end())
	{
		// TODO: MSYS2 cross-compile from linux

		auto& list = m_platformRequires.at(m_system);
		switch (m_system)
		{
			case SupportedSystem::LinuxArch:
			case SupportedSystem::LinuxManjaro: {
				if (!checkDependenciesWithPacman(list, errors))
					return false;
				break;
			}
			case SupportedSystem::LinuxDebian:
			case SupportedSystem::LinuxUbuntu: {
				if (!checkDependenciesWithApt(list, errors))
					return false;
				break;
			}
			case SupportedSystem::LinuxFedora:
			case SupportedSystem::LinuxRedHat: {
				if (!checkDependenciesWithYum(list, errors))
					return false;
				break;
			}
			default: break;
		}
	}
#endif

	if (!errors.empty())
	{
		for (auto& file : m_cachedFiles)
			Files::removeIfExists(file);

		for (auto& error : errors)
		{
			Diagnostic::error(error);
		}
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

/*****************************************************************************/
void PlatformDependencyManager::showInfo(const char* inPackageMgr, const bool inEllipses) const
{
	static const char prefix[] = "Verifying required ";
	static const char suffix[] = " packages";

	if (inEllipses)
		Diagnostic::infoEllipsis("{}{}{}", prefix, inPackageMgr, suffix);
	else
		Diagnostic::info("{}{}{}", prefix, inPackageMgr, suffix);
}

/*****************************************************************************/
bool PlatformDependencyManager::isSupportedSystemValid(const SupportedSystem supportedSystem) const
{
	if (m_system == SupportedSystem::WindowsMSYS2)
	{
		return supportedSystem == SupportedSystem::WindowsMSYS2;
	}
	else if (m_system == SupportedSystem::MacOS)
	{
		return supportedSystem == SupportedSystem::MacOSHomebrew || supportedSystem == SupportedSystem::MacOSMacPorts;
	}
	else
	{
		return m_system != SupportedSystem::Unknown && supportedSystem == m_system;
	}
}

/*****************************************************************************/
PlatformDependencyManager::SupportedSystem PlatformDependencyManager::getSupportedSystemFromString(const std::string& inKind) const noexcept
{
#if defined(CHALET_WIN32)
	if (String::equals(Keys::ReqWindowsMSYS2, inKind))
		return SupportedSystem::WindowsMSYS2;
#elif defined(CHALET_MACOS)
	if (String::equals(Keys::ReqMacOSHomebrew, inKind))
		return SupportedSystem::MacOSHomebrew;

	if (String::equals(Keys::ReqMacOSMacPorts, inKind))
		return SupportedSystem::MacOSMacPorts;
#else
	if (String::equals(Keys::ReqArchLinuxSystem, inKind))
		return SupportedSystem::LinuxArch;
	if (String::equals(Keys::ReqManjaroSystem, inKind))
		return SupportedSystem::LinuxManjaro;

	if (String::equals(Keys::ReqDebianSystem, inKind))
		return SupportedSystem::LinuxDebian;
	if (String::equals(Keys::ReqUbuntuSystem, inKind))
		return SupportedSystem::LinuxUbuntu;

	if (String::equals(Keys::ReqFedoraSystem, inKind))
		return SupportedSystem::LinuxFedora;
	if (String::equals(Keys::ReqRedHatSystem, inKind))
		return SupportedSystem::LinuxRedHat;
#endif

	return SupportedSystem::Unknown;
}

/*****************************************************************************/
void PlatformDependencyManager::getDataWithCache(std::string& outData, const StringList& inList, const std::function<std::string()>& onGet) const
{
	auto& buildDir = m_state.paths.buildOutputDir();
	auto joinedList = String::join(inList);
	auto hashedFile = m_state.cache.getHashPath(fmt::format("deps_{}_{}_{}.txt", m_systemString, buildDir, joinedList));

	if (!Files::pathExists(hashedFile))
	{
		outData = onGet();
		Files::ofstream(hashedFile) << outData;

		m_state.cache.file().addExtraHash(String::getPathFilename(hashedFile));
	}
	else
	{
		outData = Files::getFileContents(hashedFile);
		m_state.cache.file().addExtraHash(String::getPathFilename(hashedFile));
	}
	m_cachedFiles.emplace_back(std::move(hashedFile));
}

#if defined(CHALET_WIN32)
/*****************************************************************************/
bool PlatformDependencyManager::checkDependenciesWithPacmanMSYS2(const StringList& inList, StringList& errors) const
{
	auto& cc = m_state.toolchain.compilerCxxAny();

	auto detect = String::getPathFolder(cc.path);
	detect = Files::getCanonicalPath(fmt::format("{}/../../usr/bin/msys-2.0.dll", detect));
	if (!Files::pathExists(detect))
		return true; // if toolchain is not MSYS2 (could be some other MinGW version) ... skip

	Timer timer;

	showInfo("MSYS2");

	std::string installed;
	getDataWithCache(installed, inList, [this, &detect, &inList]() {
		auto pacman = fmt::format("{}/pacman.exe", String::getPathFolder(detect));
		if (Files::pathExists(pacman))
		{
			StringList cmd{ pacman, "-Q" };
			for (auto& item : inList)
			{
				cmd.emplace_back(item);
				cmd.emplace_back(fmt::format("mingw-w64-{}-{}", m_archString, item));
			}

			auto result = Process::runOutput(cmd);
			return result;
		}
		else
		{
			return std::string();
		}
	});

	if (installed.empty())
	{
		Diagnostic::error("There was a problem detecting the MSYS2 dependencies.");
		return false;
	}

	Diagnostic::printDone(timer.asString());

	installed = fmt::format("\n{}", installed);

	StringList notFound;
	for (auto& item : inList)
	{
		auto findA = fmt::format("\n{} ", item);
		auto findB = fmt::format("\nmingw-w64-{}-{} ", m_archString, item);

		bool existsA = String::contains(findA, installed);
		bool existsB = String::contains(findB, installed);

		bool exists = existsB || existsA;

		if (existsB)
			Diagnostic::subInfoEllipsis("mingw-w64-{}-{}", m_archString, item);
		else
			Diagnostic::subInfoEllipsis("{}", item);

		Diagnostic::printFound(exists);

		if (!exists)
		{
			notFound.emplace_back(item);
		}
	}

	if (!notFound.empty())
	{
		for (auto& package : notFound)
		{
			errors.push_back(fmt::format("Run: sudo port install {}", package));
		}
	}

	return true;
}
#elif defined(CHALET_MACOS)
/*****************************************************************************/
bool PlatformDependencyManager::checkDependenciesWithHomebrew(const StringList& inList, StringList& errors) const
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

	showInfo("Homebrew", false);

	StringList notFound;
	for (auto& item : inList)
	{
		Diagnostic::subInfoEllipsis("{}", item);

		auto path = fmt::format("/usr/local/Cellar/{}", item);
		bool exists = Files::pathExists(path) && Files::pathIsDirectory(path);
		Diagnostic::printFound(exists);

		if (!exists)
		{
			notFound.emplace_back(item);
		}
	}

	if (!notFound.empty())
	{
		errors.push_back(fmt::format("Run: brew install {}", String::join(notFound)));
	}

	return true;
}

/*****************************************************************************/
bool PlatformDependencyManager::checkDependenciesWithMacPorts(const StringList& inList, StringList& errors) const
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

	showInfo("MacPorts");

	auto installed = Process::runOutput({ port, "installed" });
	Diagnostic::printDone(timer.asString());

	if (installed.empty())
	{
		Diagnostic::error("There was a problem detecting the MacPorts dependencies.");
		return false;
	}

	StringList notFound;
	for (auto& item : inList)
	{
		Diagnostic::subInfoEllipsis("{}", item);

		auto find = fmt::format("\n  {} ", item);

		bool exists = String::contains(find, installed);
		Diagnostic::printFound(exists);

		if (!exists)
		{
			notFound.emplace_back(item);
		}
	}

	if (!notFound.empty())
	{
		errors.push_back(fmt::format("Run: pacman -S {}", String::join(notFound)));
	}

	return true;
}

#else
/*****************************************************************************/
bool PlatformDependencyManager::checkDependenciesWithPacman(const StringList& inList, StringList& errors) const
{
	Timer timer;

	showInfo("system");

	std::string installed;
	getDataWithCache(installed, inList, [this, &inList]() {
		auto pacman = Files::which("pacman");
		if (Files::pathExists(pacman))
		{
			StringList cmd{ pacman, "-Q" };
			for (auto& item : inList)
				cmd.emplace_back(item);

			auto result = Process::runOutput(cmd);
			return result;
		}
		else
		{
			return std::string();
		}
	});

	if (installed.empty())
	{
		Diagnostic::error("There was a problem detecting the system dependencies.");
		return false;
	}

	Diagnostic::printDone(timer.asString());

	installed = fmt::format("\n{}", installed);

	StringList notFound;
	for (auto& item : inList)
	{
		Diagnostic::subInfoEllipsis("{}", item);

		auto find = fmt::format("\n{} ", item);

		bool exists = String::contains(find, installed);
		Diagnostic::printFound(exists);

		if (!exists)
		{
			notFound.emplace_back(item);
		}
	}

	if (!notFound.empty())
	{
		errors.push_back(fmt::format("Run: sudo pacman -S {}", String::join(notFound)));
	}

	return true;
}

/*****************************************************************************/
bool PlatformDependencyManager::checkDependenciesWithApt(const StringList& inList, StringList& errors) const
{
	Timer timer;

	showInfo("system");

	std::string installed;
	getDataWithCache(installed, inList, [this, &inList]() {
		auto apt = Files::which("apt");
		if (Files::pathExists(apt))
		{
			StringList cmd{ apt, "list", "--installed" };
			for (auto& item : inList)
				cmd.emplace_back(item);

			auto result = Process::runOutput(cmd);
			return result;
		}
		else
		{
			return std::string();
		}
	});

	if (installed.empty())
	{
		Diagnostic::error("There was a problem detecting the system dependencies.");
		return false;
	}

	Diagnostic::printDone(timer.asString());

	auto split = String::split(installed, '\n');

	StringList foundItems;

	for (auto& line : split)
	{
		if (line.empty())
			continue;

		for (auto& item : inList)
		{
			if (String::startsWith(fmt::format("{}/", item), line)
				&& (String::contains(fmt::format(" {} [", m_archString), line) || String::contains(" all [", line)))
				foundItems.emplace_back(item);
		}
	}

	StringList notFound;
	for (auto& item : inList)
	{
		Diagnostic::subInfoEllipsis("{}", item);

		bool exists = List::contains(foundItems, item);
		Diagnostic::printFound(exists);

		if (!exists)
		{
			notFound.emplace_back(item);
		}
	}

	if (!notFound.empty())
	{
		errors.push_back(fmt::format("Run: sudo apt install {}", String::join(notFound)));
	}

	return true;
}

/*****************************************************************************/
bool PlatformDependencyManager::checkDependenciesWithYum(const StringList& inList, StringList& errors) const
{
	Timer timer;

	showInfo("system");

	std::string installed;
	getDataWithCache(installed, inList, [this, &inList]() {
		auto yum = Files::which("yum");
		if (Files::pathExists(yum))
		{
			StringList cmd{ yum, "list", "--installed", "--color=off" };
			for (auto& item : inList)
				cmd.emplace_back(item);

			auto result = Process::runOutput(cmd);
			return result;
		}
		else
		{
			return std::string();
		}
	});

	if (installed.empty())
	{
		Diagnostic::error("There was a problem detecting the system dependencies.");
		return false;
	}

	Diagnostic::printDone(timer.asString());

	StringList notFound;
	for (auto& item : inList)
	{
		Diagnostic::subInfoEllipsis("{}", item);

		auto findA = fmt::format("\n{}.{}", item, m_archString);
		auto findB = fmt::format("\n{}.noarch", item);

		bool exists = String::contains(findA, installed) || String::contains(findB, installed);
		Diagnostic::printFound(exists);

		if (!exists)
		{
			notFound.emplace_back(item);
		}
	}

	if (!notFound.empty())
	{
		errors.push_back(fmt::format("Run: sudo apt install {}", String::join(notFound)));
	}

	return true;
}
#endif
}
