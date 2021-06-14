/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/CacheTools.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/DependencyWalker.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
bool CacheTools::resolveOwnExecutable(const std::string& inAppPath)
{
	if (m_chalet.empty())
	{
		m_chalet = inAppPath;

		if (!Commands::pathExists(m_chalet))
		{
			m_chalet = Commands::which("chalet");
			if (!Commands::pathExists(m_chalet))
				m_chalet.clear();
		}
	}

	return true;
}

/*****************************************************************************/
void CacheTools::fetchBashVersion()
{
	if (!m_bash.empty())
	{
#if defined(CHALET_WIN32)
		/*if (Commands::pathExists(m_bash))
		{
			std::string version = Commands::subprocessOutput({ m_bash, "--version" });
			m_bashAvailable = String::startsWith("GNU bash", version);
		}*/
		m_bashAvailable = Commands::pathExists(m_bash);
#else
		m_bashAvailable = true;
#endif
	}
}

/*****************************************************************************/
void CacheTools::fetchBrewVersion()
{
#if defined(CHALET_MACOS)
	if (!m_brew.empty())
	{
		/*if (Commands::pathExists(m_brew))
		{
			std::string version = Commands::subprocessOutput({ m_brew, "--version" });
			m_brewAvailable = String::startsWith("Homebrew ", version);
		}*/
		m_brewAvailable = Commands::pathExists(m_brew);
	}
#endif
}

/*****************************************************************************/
void CacheTools::fetchXcodeVersion()
{
#if defined(CHALET_MACOS)
	if (!m_xcodebuild.empty() && m_xcodeVersionMajor == 0 && m_xcodeVersionMinor == 0)
	{
		if (Commands::pathExists(m_xcodebuild))
		{
			std::string version = Commands::subprocessOutput({ m_xcodebuild, "-version" });
			if (String::contains("requires Xcode", version))
				return;

			version = Commands::isolateVersion(version);

			auto vals = String::split(version, '.');
			if (vals.size() == 2)
			{
				m_xcodeVersionMajor = std::stoi(vals[0]);
				m_xcodeVersionMinor = std::stoi(vals[1]);
			}
		}
	}
#endif
}

/*****************************************************************************/
void CacheTools::fetchXcodeGenVersion()
{
#if defined(CHALET_MACOS)
	if (!m_xcodegen.empty() && m_xcodegenVersionMajor == 0 && m_xcodegenVersionMinor == 0)
	{
		if (Commands::pathExists(m_xcodegen))
		{
			std::string version = Commands::subprocessOutput({ m_xcodegen, "--version" });
			version = Commands::isolateVersion(version);

			auto vals = String::split(version, '.');
			if (vals.size() == 3)
			{
				m_xcodegenVersionMajor = std::stoi(vals[0]);
				m_xcodegenVersionMinor = std::stoi(vals[1]);
				m_xcodegenVersionPatch = std::stoi(vals[2]);
			}
		}
	}
#endif
}

/*****************************************************************************/
const std::string& CacheTools::chalet() const noexcept
{
	return m_chalet;
}

/*****************************************************************************/
const std::string& CacheTools::bash() const noexcept
{
	return m_bash;
}

void CacheTools::setBash(std::string&& inValue) noexcept
{
	m_bash = std::move(inValue);
}

bool CacheTools::bashAvailable() const noexcept
{
	return m_bashAvailable;
}

/*****************************************************************************/
const std::string& CacheTools::brew() const noexcept
{
	return m_brew;
}
void CacheTools::setBrew(std::string&& inValue) noexcept
{
	m_brew = std::move(inValue);
}
bool CacheTools::brewAvailable() const noexcept
{
	return m_brewAvailable;
}

/*****************************************************************************/
const std::string& CacheTools::codesign() const noexcept
{
	return m_codesign;
}
void CacheTools::setCodesign(std::string&& inValue) noexcept
{
	m_codesign = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::commandPrompt() const noexcept
{
	return m_commandPrompt;
}
void CacheTools::setCommandPrompt(std::string&& inValue) noexcept
{
	m_commandPrompt = std::move(inValue);
	Path::sanitizeForWindows(m_commandPrompt);
}

/*****************************************************************************/
const std::string& CacheTools::git() const noexcept
{
	return m_git;
}
void CacheTools::setGit(std::string&& inValue) noexcept
{
	m_git = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::hdiutil() const noexcept
{
	return m_hdiutil;
}
void CacheTools::setHdiutil(std::string&& inValue) noexcept
{
	m_hdiutil = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::installNameTool() const noexcept
{
	return m_installNameTool;
}
void CacheTools::setInstallNameTool(std::string&& inValue) noexcept
{
	m_installNameTool = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::instruments() const noexcept
{
	return m_instruments;
}
void CacheTools::setInstruments(std::string&& inValue) noexcept
{
	m_instruments = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::ldd() const noexcept
{
	return m_ldd;
}
void CacheTools::setLdd(std::string&& inValue) noexcept
{
	m_ldd = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::lipo() const noexcept
{
	return m_lipo;
}
void CacheTools::setLipo(std::string&& inValue) noexcept
{
	m_lipo = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::lua() const noexcept
{
	return m_lua;
}
void CacheTools::setLua(std::string&& inValue) noexcept
{
	m_lua = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::applePlatformSdk(const std::string& inKey) const
{
	return m_applePlatformSdk.at(inKey);
}
void CacheTools::addApplePlatformSdk(const std::string& inKey, std::string&& inValue)
{
	m_applePlatformSdk[inKey] = std::move(inValue);
}

/*****************************************************************************/
const std::string CacheTools::osascript() const noexcept
{
	return m_osascript;
}

void CacheTools::setOsascript(std::string&& inValue) noexcept
{
	m_osascript = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::otool() const noexcept
{
	return m_otool;
}
void CacheTools::setOtool(std::string&& inValue) noexcept
{
	m_otool = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::perl() const noexcept
{
	return m_perl;
}
void CacheTools::setPerl(std::string&& inValue) noexcept
{
	m_perl = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::plutil() const noexcept
{
	return m_plutil;
}
void CacheTools::setPlutil(std::string&& inValue) noexcept
{
	m_plutil = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::powershell() const noexcept
{
	return m_powershell;
}
void CacheTools::setPowershell(std::string&& inValue) noexcept
{
	m_powershell = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::python() const noexcept
{
	return m_python;
}
void CacheTools::setPython(std::string&& inValue) noexcept
{
	m_python = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::python3() const noexcept
{
	return m_python3;
}
void CacheTools::setPython3(std::string&& inValue) noexcept
{
	m_python3 = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::ruby() const noexcept
{
	return m_ruby;
}
void CacheTools::setRuby(std::string&& inValue) noexcept
{
	m_ruby = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::sample() const noexcept
{
	return m_sample;
}
void CacheTools::setSample(std::string&& inValue) noexcept
{
	m_sample = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::sips() const noexcept
{
	return m_sips;
}
void CacheTools::setSips(std::string&& inValue) noexcept
{
	m_sips = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::tiffutil() const noexcept
{
	return m_tiffutil;
}
void CacheTools::setTiffutil(std::string&& inValue) noexcept
{
	m_tiffutil = std::move(inValue);
}

/*****************************************************************************/
const std::string& CacheTools::xcodebuild() const noexcept
{
	return m_xcodebuild;
}
void CacheTools::setXcodebuild(std::string&& inValue) noexcept
{
	m_xcodebuild = std::move(inValue);
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
void CacheTools::setXcodegen(std::string&& inValue) noexcept
{
	m_xcodegen = std::move(inValue);
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
void CacheTools::setXcrun(std::string&& inValue) noexcept
{
	m_xcrun = std::move(inValue);
}

/*****************************************************************************/
std::string CacheTools::getAsmGenerateCommand(const std::string& inputFile, const std::string& outputFile) const
{
	// TODO: Customizations for these commands
#if defined(CHALET_MACOS)
	return fmt::format("{otool} -tvV {inputFile} | c++filt > {outputFile}",
		fmt::arg("otool", m_otool),
		FMT_ARG(inputFile),
		FMT_ARG(outputFile));
#else
	return fmt::format("objdump -d -C -Mintel {inputFile} > {outputFile}",
		FMT_ARG(inputFile),
		FMT_ARG(outputFile));
#endif
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
#if defined(CHALET_WIN32)
	DependencyWalker depsWalker;
	if (!depsWalker.read(inPath, outList))
	{
		Diagnostic::error(fmt::format("Dependencies for file '{}' could not be read.", inPath));
		return false;
	}

	return true;
#else
	#if defined(CHALET_MACOS)
	if (m_otool.empty())
	{
		Diagnostic::error(fmt::format("Dependencies for file '{}' could not be read. 'otool' was not found in cache.", inPath));
		return false;
	}
	#else
	if (m_ldd.empty())
	{
		Diagnostic::error(fmt::format("Dependencies for file '{}' could not be read. 'ldd' was not found in cache.", inPath));
		return false;
	}
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

			if (String::startsWith("Archive", line))
				break;

			if (String::startsWith(inPath, line))
				continue;

	#if defined(CHALET_MACOS)
			if (!String::contains(".dylib", line))
				continue;

			std::size_t end = line.find(".dylib") + 6;
	#else
			std::size_t end = line.find(" => ");
	#endif
			if (end == std::string::npos)
				continue;

			while (line[beg] == '\t')
				beg++;

			std::string dependency = line.substr(beg, end - beg);
			// rpath, executable_path, etc
			if (String::startsWith('@', dependency))
			{
				auto firstSlash = dependency.find('/');
				if (firstSlash != std::string::npos)
					dependency = dependency.substr(firstSlash + 1);
			}

			if (String::startsWith("/usr/lib", dependency))
				continue;

			List::addIfDoesNotExist(outList, std::move(dependency));
		}

		return true;
	}
	catch (const std::runtime_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
#endif
}

}
