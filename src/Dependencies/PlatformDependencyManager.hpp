/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;

struct PlatformDependencyManager
{
	explicit PlatformDependencyManager(const BuildState& inState);

	bool initialize();

	void addRequiredDependency(const std::string& inKind, std::string&& inValue);
	void addRequiredDependency(const std::string& inKind, StringList&& inValue);

	bool validate();

private:
	enum class SupportedSystem
	{
		Unknown,
		LinuxDebian,
		LinuxUbuntu,
		LinuxFedora,
		LinuxRedHat,
		LinuxArch,
		LinuxManjaro,
		MacOS,
		MacOSHomebrew,
		MacOSMacPorts,
		Windows,
		WindowsMSYS2,
	};

	void showInfo(const char* inPackageMgr, const bool inEllipses = true) const;
	bool isSupportedSystemValid(const SupportedSystem supportedSystem) const;
	SupportedSystem getSupportedSystemFromString(const std::string& inKind) const noexcept;

	void getDataWithCache(std::string& outData, const StringList& inList, const std::function<std::string()>& onGet) const;

#if defined(CHALET_WIN32)
	bool checkDependenciesWithPacmanMSYS2(const StringList& inList, StringList& errors) const;
#elif defined(CHALET_MACOS)
	bool checkDependenciesWithHomebrew(const StringList& inList, StringList& errors) const;
	bool checkDependenciesWithMacPorts(const StringList& inList, StringList& errors) const;
#else
	bool checkDependenciesWithPacman(const StringList& inList, StringList& errors) const;
	bool checkDependenciesWithApt(const StringList& inList, StringList& errors) const;
	bool checkDependenciesWithYum(const StringList& inList, StringList& errors) const;
#endif

	const BuildState& m_state;

	mutable StringList m_cachedFiles;

	std::string m_systemString;
	std::string m_archString;

	std::unordered_map<SupportedSystem, StringList> m_platformRequires;

	SupportedSystem m_system = SupportedSystem::Unknown;
};
}
