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
const std::string& CacheTools::bash() const noexcept
{
	return m_bash;
}

void CacheTools::setBash(const std::string& inValue) noexcept
{
	m_bash = inValue;

#if defined(CHALET_WIN32)
	if (Commands::pathExists(m_bash))
	{
		std::string version = Commands::subprocessOutput({ m_bash, "--version" });
		m_bashAvailable = String::contains("GNU bash", version);
	}
#else
	m_bashAvailable = true;
#endif
}

bool CacheTools::bashAvailable() noexcept
{
	return m_bashAvailable;
}

/*****************************************************************************/
const std::string& CacheTools::brew() const noexcept
{
	return m_brew;
}
void CacheTools::setBrew(const std::string& inValue) noexcept
{
	m_brew = inValue;

#if defined(CHALET_MACOS)
	if (Commands::pathExists(m_brew))
	{
		std::string version = Commands::subprocessOutput({ m_brew, "--version" });
		auto firstEol = version.find('\n');
		if (firstEol != std::string::npos)
		{
			version = version.substr(0, firstEol);
		}

		m_brewAvailable = String::startsWith("Homebrew ", version);
	}
#endif
}
bool CacheTools::brewAvailable() noexcept
{
	return m_brewAvailable;
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
const std::string& CacheTools::codesign() const noexcept
{
	return m_codesign;
}
void CacheTools::setCodesign(const std::string& inValue) noexcept
{
	m_codesign = inValue;
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
const std::string& CacheTools::gprof() const noexcept
{
	return m_gprof;
}
void CacheTools::setGprof(const std::string& inValue) noexcept
{
	m_gprof = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::hdiutil() const noexcept
{
	return m_hdiutil;
}
void CacheTools::setHdiutil(const std::string& inValue) noexcept
{
	m_hdiutil = inValue;
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
const std::string& CacheTools::instruments() const noexcept
{
	return m_instruments;
}
void CacheTools::setInstruments(const std::string& inValue) noexcept
{
	m_instruments = inValue;
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
	m_make = inValue;

	if (Commands::pathExists(m_make))
	{
		std::string version = Commands::subprocessOutput({ m_make, "--version" });
		isolateVersion(version);

		auto vals = String::split(version, '.');
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
const std::string& CacheTools::ninja() const noexcept
{
	return m_ninja;
}
void CacheTools::setNinja(const std::string& inValue) noexcept
{
	m_ninja = inValue;
}

/*****************************************************************************/
const std::string CacheTools::osascript() const noexcept
{
	return m_osascript;
}

void CacheTools::setOsascript(const std::string& inValue) noexcept
{
	m_osascript = inValue;
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
const std::string& CacheTools::plutil() const noexcept
{
	return m_plutil;
}
void CacheTools::setPlutil(const std::string& inValue) noexcept
{
	m_plutil = inValue;
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
const std::string& CacheTools::sample() const noexcept
{
	return m_sample;
}
void CacheTools::setSample(const std::string& inValue) noexcept
{
	m_sample = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::sips() const noexcept
{
	return m_sips;
}
void CacheTools::setSips(const std::string& inValue) noexcept
{
	m_sips = inValue;
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
const std::string& CacheTools::tiffutil() const noexcept
{
	return m_tiffutil;
}
void CacheTools::setTiffutil(const std::string& inValue) noexcept
{
	m_tiffutil = inValue;
}

/*****************************************************************************/
const std::string& CacheTools::xcodebuild() const noexcept
{
	return m_xcodebuild;
}
void CacheTools::setXcodebuild(const std::string& inValue) noexcept
{
	m_xcodebuild = inValue;

#if defined(CHALET_MACOS)
	if (Commands::pathExists(m_xcodebuild))
	{
		std::string version = Commands::subprocessOutput({ m_xcodebuild, "-version" });
		if (String::contains("requires Xcode", version))
			return;

		isolateVersion(version);

		auto vals = String::split(version, '.');
		if (vals.size() == 2)
		{
			m_xcodeVersionMajor = std::stoi(vals[0]);
			m_xcodeVersionMinor = std::stoi(vals[1]);
		}
	}
#endif
}
uint CacheTools::xcodeVersionMajor() const noexcept
{
	return m_xcodeVersionMajor;
}
uint CacheTools::xcodeVersionMinor() const noexcept
{
	return m_xcodeVersionMinor;
}

/*****************************************************************************/
const std::string& CacheTools::xcodegen() const noexcept
{
	return m_xcodegen;
}
void CacheTools::setXcodegen(const std::string& inValue) noexcept
{
	m_xcodegen = inValue;

#if defined(CHALET_MACOS)
	if (Commands::pathExists(m_xcodegen))
	{
		std::string version = Commands::subprocessOutput({ m_xcodegen, "--version" });
		isolateVersion(version);

		auto vals = String::split(version, '.');
		if (vals.size() == 3)
		{
			m_xcodegenVersionMajor = std::stoi(vals[0]);
			m_xcodegenVersionMinor = std::stoi(vals[1]);
			m_xcodegenVersionPatch = std::stoi(vals[2]);
		}
	}
#endif
}
uint CacheTools::xcodegenVersionMajor() const noexcept
{
	return m_xcodegenVersionMajor;
}
uint CacheTools::xcodegenVersionMinor() const noexcept
{
	return m_xcodegenVersionMinor;
}
uint CacheTools::xcodegenVersionPatch() const noexcept
{
	return m_xcodegenVersionPatch;
}

/*****************************************************************************/
const std::string& CacheTools::xcrun() const noexcept
{
	return m_xcrun;
}
void CacheTools::setXcrun(const std::string& inValue) noexcept
{
	m_xcrun = inValue;
}

/*****************************************************************************/
bool CacheTools::installHomebrewPackage(const std::string& inPackage, const bool inCleanOutput) const
{
#if defined(CHALET_MACOS)
	const std::string result = Commands::subprocessOutput({ m_brew, "ls", "--versions", inPackage }, inCleanOutput);
	if (result.empty())
	{
		if (!Commands::subprocess({ m_brew, "install", inPackage }, inCleanOutput))
			return false;
	}

	return true;
#else
	UNUSED(inPackage, inCleanOutput);
	return false;
#endif
}

/*****************************************************************************/
std::string CacheTools::getCurrentGitRepositoryBranch(const std::string& inRepoPath, const bool inCleanOutput) const
{
	std::string branch = Commands::subprocessOutput({ m_git, "-C", inRepoPath, "rev-parse", "--abbrev-ref", "HEAD" }, inCleanOutput);
	return branch;
}

/*****************************************************************************/
std::string CacheTools::getCurrentGitRepositoryTag(const std::string& inRepoPath, const bool inCleanOutput) const
{
	std::string tag = Commands::subprocessOutput({ m_git, "-C", inRepoPath, "describe", "--tags", "--exact-match", "abbrev=0" }, inCleanOutput, PipeOption::Close);
	return tag;
}

/*****************************************************************************/
std::string CacheTools::getCurrentGitRepositoryHash(const std::string& inRepoPath, const bool inCleanOutput) const
{
	std::string hash = Commands::subprocessOutput({ m_git, "-C", inRepoPath, "rev-parse", "--verify", "--quiet", "HEAD" }, inCleanOutput);
	return hash;
}

/*****************************************************************************/
std::string CacheTools::getCurrentGitRepositoryHashFromRemote(const std::string& inRepoPath, const std::string& inBranch, const bool inCleanOutput) const
{
	std::string originHash = Commands::subprocessOutput({ m_git, "-C", inRepoPath, "rev-parse", "--verify", "--quiet", fmt::format("origin/{}", inBranch) }, inCleanOutput);
	return originHash;
}

/*****************************************************************************/
bool CacheTools::updateGitRepositoryShallow(const std::string& inRepoPath, const bool inCleanOutput) const
{
	return Commands::subprocess({ m_git, "-C", inRepoPath, "pull", "--quiet", "--update-shallow" }, inCleanOutput);
}

/*****************************************************************************/
bool CacheTools::resetGitRepositoryToCommit(const std::string& inRepoPath, const std::string& inCommit, const bool inCleanOutput) const
{
	return Commands::subprocess({ m_git, "-C", inRepoPath, "reset", "--quiet", "--hard", inCommit }, inCleanOutput);
}

/*****************************************************************************/
bool CacheTools::plistConvertToBinary(const std::string& inInput, const std::string& inOutput, const bool inCleanOutput) const
{
#if defined(CHALET_MACOS)
	return Commands::subprocess({ m_plutil, "-convert", "binary1", inInput, "-o", inOutput }, inCleanOutput);
#else
	UNUSED(inInput, inOutput, inCleanOutput);
	return false;
#endif
}

/*****************************************************************************/
bool CacheTools::plistReplaceProperty(const std::string& inPlistFile, const std::string& inKey, const std::string& inValue, const bool inCleanOutput) const
{
#if defined(CHALET_MACOS)
	return Commands::subprocess({ m_plutil, "-replace", inKey, "-string", inValue, inPlistFile }, inCleanOutput);
#else
	UNUSED(inPlistFile, inKey, inValue, inCleanOutput);
	return false;
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
		StringList cmd;
#if defined(CHALET_MACOS)
		cmd = { m_otool, "-L", inPath };
#else
		// This block detects the dependencies of each target and adds them to a list
		// The list resolves each path, favoring the paths supplied by build.json
		// Note: this doesn't seem to work in standalone builds of GCC (tested 7.3.0)
		//   but works fine w/ MSYS2
		cmd = { m_ldd, inPath };
#endif
		std::string targetDeps = Commands::subprocessOutput(cmd);

		std::string line;
		std::istringstream stream(targetDeps);

		while (std::getline(stream, line))
		{
			std::size_t beg = 0;
#if defined(CHALET_MACOS)
			std::size_t end = line.find(".dylib") + 5;
#else
	#if defined(CHALET_WIN32)
			if (String::contains("System32", line) || String::contains("SYSTEM32", line) || String::contains("SysWOW64", line) || String::contains("SYSWOW64", line))
				continue;
	#else
			if (String::contains("/usr/lib", line))
				continue;
	#endif
			std::size_t end = line.find(" => ");
#endif
			if (end == std::string::npos)
				continue;

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

/*****************************************************************************/
void CacheTools::isolateVersion(std::string& outString)
{
	auto firstEol = outString.find('\n');
	if (firstEol != std::string::npos)
	{
		outString = outString.substr(0, firstEol);
	}

	auto lastSpace = outString.find_last_of(' ');
	if (lastSpace != std::string::npos)
	{
		outString = outString.substr(lastSpace + 1);
	}
}

}
