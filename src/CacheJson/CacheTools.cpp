/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "CacheJson/CacheTools.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
const std::string& CacheTools::ar() const noexcept
{
	return m_ar;
}
void CacheTools::setAr(const std::string& inValue) noexcept
{
	m_ar = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::brew() const noexcept
{
	return m_brew;
}
void CacheTools::setBrew(const std::string& inValue) noexcept
{
	m_brew = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::cmake() const noexcept
{
	return m_cmake;
}
void CacheTools::setCmake(const std::string& inValue) noexcept
{
	m_cmake = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::codeSign() const noexcept
{
	return m_codeSign;
}
void CacheTools::setCodeSign(const std::string& inValue) noexcept
{
	m_codeSign = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::git() const noexcept
{
	return m_git;
}
void CacheTools::setGit(const std::string& inValue) noexcept
{
	m_git = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::hdiUtil() const noexcept
{
	return m_hdiUtil;
}
void CacheTools::setHdiUtil(const std::string& inValue) noexcept
{
	m_hdiUtil = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::installNameUtil() const noexcept
{
	return m_installNameUtil;
}
void CacheTools::setInstallNameUtil(const std::string& inValue) noexcept
{
	m_installNameUtil = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::ldd() const noexcept
{
	return m_ldd;
}
void CacheTools::setLdd(const std::string& inValue) noexcept
{
	m_ldd = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::macosSdk() const noexcept
{
	return m_macosSdk;
}
void CacheTools::setMacosSdk(const std::string& inValue) noexcept
{
	m_macosSdk = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::make() const noexcept
{
	return m_make;
}
void CacheTools::setMake(const std::string& inValue) noexcept
{
#if defined(CHALET_WIN32)
	m_make = String::getPathBaseName(inValue);
#else
	m_make = inValue;
#endif

	if (Commands::pathExists(m_make))
	{
		std::string version = Commands::shellWithOutput(fmt::format("{} --version | grep -e 'GNU Make'", m_make));
		String::replaceAll(version, "GNU Make ", "");
		auto vals = String::split(version, ".");
		if (vals.size() == 2)
		{
			m_makeVersionMajor = std::stoi(vals[0]);
			m_makeVersionMinor = std::stoi(vals[1]);
		}
	}
}

uint CacheTools::makeVersionMajor() const noexcept
{
	return m_makeVersionMajor;
}
uint CacheTools::makeVersionMinor() const noexcept
{
	return m_makeVersionMinor;
}

/*****************************************************************************/
const std::string& CacheTools::makeIcns() const noexcept
{
	return m_makeIcns;
}
void CacheTools::setMakeIcns(const std::string& inValue) noexcept
{
	m_makeIcns = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::ninja() const noexcept
{
	return m_ninja;
}
void CacheTools::setNinja(const std::string& inValue) noexcept
{
	m_ninja = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::otool() const noexcept
{
	return m_otool;
}
void CacheTools::setOtool(const std::string& inValue) noexcept
{
	m_otool = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::plUtil() const noexcept
{
	return m_plUtil;
}
void CacheTools::setPlUtil(const std::string& inValue) noexcept
{
	m_plUtil = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::ranlib() const noexcept
{
	return m_ranlib;
}
void CacheTools::setRanlib(const std::string& inValue) noexcept
{
	m_ranlib = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::strip() const noexcept
{
	return m_strip;
}
void CacheTools::setStrip(const std::string& inValue) noexcept
{
	m_strip = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::tiffUtil() const noexcept
{
	return m_tiffUtil;
}
void CacheTools::setTiffUtil(const std::string& inValue) noexcept
{
	m_tiffUtil = inValue;
}

/*****************************************************************************/
bool CacheTools::installHomebrewPackage(const std::string& inPackage, const bool inCleanOutput) const
{
#if defined(CHALET_MACOS)
	std::string command = fmt::format("{brew} ls --versions {inPackage}",
		fmt::arg("brew", m_brew),
		FMT_ARG(inPackage));

	const std::string result = Commands::shellWithOutput(command, inCleanOutput);
	if (result.empty())
	{
		command = fmt::format("{brew} install {inPackage}",
			fmt::arg("brew", m_brew),
			FMT_ARG(inPackage));

		// brew install makeicns
		if (!Commands::shell(command, inCleanOutput))
			return false;
	}

	return true;
#else
	UNUSED(inPackage, inCleanOutput);
	return true;
#endif
}

/*****************************************************************************/
std::string CacheTools::getCurrentGitRepositoryBranch(const std::string& inRepoPath, const bool inCleanOutput) const
{
	const std::string cmd = fmt::format("\"{git}\" -C {inRepoPath} rev-parse --abbrev-ref HEAD",
		fmt::arg("git", m_git),
		FMT_ARG(inRepoPath));

	// LOG(cmd);

	std::string branch = Commands::shellWithOutput(cmd, inCleanOutput);
	String::replaceAll(branch, "\n", "");

	return branch;
}

/*****************************************************************************/
std::string CacheTools::getCurrentGitRepositoryTag(const std::string& inRepoPath, const bool inCleanOutput) const
{
#if defined(CHALET_WIN32)
	const std::string null = "2> nul";
#else
	const std::string null = "2> /dev/null";
#endif
	const std::string cmd = fmt::format("\"{git}\" -C {inRepoPath} describe --tags --exact-match --abbrev=0 {null}",
		fmt::arg("git", m_git),
		FMT_ARG(inRepoPath),
		FMT_ARG(null));

	// LOG(cmd);

	std::string tag = Commands::shellWithOutput(cmd, inCleanOutput);
	String::replaceAll(tag, "\n", "");

	return tag;
}

/*****************************************************************************/
std::string CacheTools::getCurrentGitRepositoryHash(const std::string& inRepoPath, const bool inCleanOutput) const
{
	const std::string cmd = fmt::format("\"{git}\" -C {inRepoPath} rev-parse --verify --quiet HEAD",
		fmt::arg("git", m_git),
		FMT_ARG(inRepoPath));

	// LOG(cmd);

	std::string hash = Commands::shellWithOutput(cmd, inCleanOutput);
	String::replaceAll(hash, "\n", "");

	return hash;
}

/*****************************************************************************/
std::string CacheTools::getCurrentGitRepositoryHashFromRemote(const std::string& inRepoPath, const std::string& inBranch, const bool inCleanOutput) const
{
	const std::string cmd = fmt::format("\"{git}\" -C {inRepoPath} rev-parse --verify --quiet origin/{inBranch}",
		fmt::arg("git", m_git),
		FMT_ARG(inRepoPath),
		FMT_ARG(inBranch));

	// LOG(cmd);

	std::string originHash = Commands::shellWithOutput(cmd, inCleanOutput);
	String::replaceAll(originHash, "\n", "");

	return originHash;
}

/*****************************************************************************/
bool CacheTools::updateGitRepositoryShallow(const std::string& inRepoPath, const bool inCleanOutput) const
{
	const std::string cmd = fmt::format("\"{git}\" -C {inRepoPath} pull --quiet --update-shallow",
		fmt::arg("git", m_git),
		FMT_ARG(inRepoPath));

	// LOG(cmd);

	return Commands::shell(cmd, inCleanOutput);
}

/*****************************************************************************/
bool CacheTools::resetGitRepositoryToCommit(const std::string& inRepoPath, const std::string& inCommit, const bool inCleanOutput) const
{
	const std::string cmd = fmt::format("\"{git}\" -C {inRepoPath} reset --quiet --hard {inCommit}",
		fmt::arg("git", m_git),
		FMT_ARG(inRepoPath),
		FMT_ARG(inCommit));

	// LOG(cmd);

	return Commands::shell(cmd, inCleanOutput);
}

/*****************************************************************************/
bool CacheTools::plistConvertToBinary(const std::string& inInput, const std::string& inOutput, const bool inCleanOutput) const
{
#if defined(CHALET_MACOS)
	const std::string cmd = fmt::format("{plutil} -convert binary1 '{inInput}' -o '{inOutput}'",
		fmt::arg("plutil", m_plUtil),
		FMT_ARG(inInput),
		FMT_ARG(inOutput));
	return Commands::shell(cmd, inCleanOutput);
#else
	UNUSED(inInput, inOutput, inCleanOutput);
	return true;
#endif
}

/*****************************************************************************/
bool CacheTools::plistReplaceProperty(const std::string& inPlistFile, const std::string& inKey, const std::string& inValue, const bool inCleanOutput) const
{
#if defined(CHALET_MACOS)
	const std::string cmd = fmt::format("{plutil} -replace '{inKey}' -string '{inValue}' '{inPlistFile}'",
		fmt::arg("plutil", m_plUtil),
		FMT_ARG(inKey),
		FMT_ARG(inValue),
		FMT_ARG(inPlistFile));
	return Commands::shell(cmd, inCleanOutput);
#else
	UNUSED(inPlistFile, inKey, inValue, inCleanOutput);
	return true;
#endif
}

/*****************************************************************************/
bool CacheTools::getExecutableDependencies(const std::string& inPath, StringList& outList) const
{
#if defined(CHALET_MACOS)
	if (m_otool.empty())
		return false;
#else
	if (m_ldd.empty())
		return false;
#endif

	try
	{

#if !defined(CHALET_MACOS)
		// This block detects the dependencies of each target and adds them to a list
		// The list resolves each path, favoring the paths supplied by build.json
		// Note: this doesn't seem to work in standalone builds of GCC (tested 7.3.0)
		//   but works fine w/ MSYS2
	#if defined(CHALET_WIN32)
		std::string cmd = fmt::format("\"{ldd}\" '{inPath}' | grep -v -e 'System32' -e 'SYSTEM32' -e 'SysWOW64' -e 'SYSWOW64'",
			fmt::arg("ldd", m_ldd),
			FMT_ARG(inPath));
	#else
		std::string cmd = fmt::format("\"{ldd}\" '{inPath}' | grep -v -e '/usr/lib'",
			fmt::arg("ldd", m_ldd),
			FMT_ARG(inPath));
	#endif
#else
		std::string cmd = fmt::format("{otool} -L '{inPath}' | grep -e 'dylib'",
			fmt::arg("otool", m_otool),
			FMT_ARG(inPath));
#endif
		std::string targetDeps = Commands::shellWithOutput(cmd);

		std::string line;
		std::istringstream stream(targetDeps);

		while (std::getline(stream, line))
		{
			std::size_t beg = 0;
#if !defined(CHALET_MACOS)
			std::size_t end = line.find(" => ");
#else
			std::size_t end = line.find(".dylib") + 5;
#endif

			while (line[beg] == '\t')
				beg++;

			std::string dependency = line.substr(beg, end);
			List::addIfDoesNotExist(outList, std::move(dependency));
		}

		return true;
	}
	catch (const std::runtime_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
}

}
