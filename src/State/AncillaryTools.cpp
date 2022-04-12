/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/AncillaryTools.hpp"

#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool AncillaryTools::resolveOwnExecutable(const std::string& inAppPath)
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
bool AncillaryTools::validate(const std::string& inHomeDirectory)
{
	fetchBashVersion();

#if defined(CHALET_MACOS)
	Environment::replaceCommonVariables(m_signingIdentity, inHomeDirectory);

	if (String::contains("${", m_signingIdentity))
	{
		RegexPatterns::matchPathVariables(m_signingIdentity, [&](std::string match) {
			if (String::equals("home", match))
				return inHomeDirectory;

			if (String::startsWith("env:", match))
			{
				match = match.substr(4);
				return Environment::getAsString(match.c_str());
			}

			return std::string();
		});
	}
#else
	UNUSED(inHomeDirectory);
#endif

	return true;
}

/*****************************************************************************/
void AncillaryTools::fetchBashVersion()
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
void AncillaryTools::fetchXcodeVersion()
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
void AncillaryTools::fetchXcodeGenVersion()
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
const std::string& AncillaryTools::applePlatformSdk(const std::string& inKey) const
{
	return m_applePlatformSdk.at(inKey);
}
void AncillaryTools::addApplePlatformSdk(const std::string& inKey, std::string&& inValue)
{
	m_applePlatformSdk[inKey] = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::chalet() const noexcept
{
	return m_chalet;
}

/*****************************************************************************/
const std::string& AncillaryTools::bash() const noexcept
{
	return m_bash;
}

void AncillaryTools::setBash(std::string&& inValue) noexcept
{
	m_bash = std::move(inValue);
}

bool AncillaryTools::bashAvailable() const noexcept
{
	return m_bashAvailable;
}

/*****************************************************************************/
const std::string& AncillaryTools::codesign() const noexcept
{
	return m_codesign;
}
void AncillaryTools::setCodesign(std::string&& inValue) noexcept
{
	m_codesign = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::signingIdentity() const noexcept
{
	return m_signingIdentity;
}
void AncillaryTools::setSigningIdentity(std::string&& inValue) noexcept
{
	m_signingIdentity = std::move(inValue);
}
bool AncillaryTools::isSigningIdentityValid() const
{
#if defined(CHALET_MACOS)
	// This can take a little bit of time (60ms), so only call it when it's needed
	if (!m_signingIdentity.empty())
	{
		// security find-identity -v -p codesigning
		auto security = Commands::which("security");
		if (!security.empty())
		{
			StringList cmd{ std::move(security), "find-identity", "-v", "-p", "codesigning" };
			auto identities = Commands::subprocessOutput(cmd);

			if (!String::contains(m_signingIdentity, identities))
			{
				Diagnostic::error("signingIdentity '{}' could not be verified with '{}'", m_signingIdentity, String::join(cmd));
				return false;
			}
		}
	}
#endif

	return true;
}

/*****************************************************************************/
const std::string& AncillaryTools::commandPrompt() const noexcept
{
	return m_commandPrompt;
}
void AncillaryTools::setCommandPrompt(std::string&& inValue) noexcept
{
	m_commandPrompt = std::move(inValue);
	Path::sanitizeForWindows(m_commandPrompt);
}

/*****************************************************************************/
const std::string& AncillaryTools::git() const noexcept
{
	return m_git;
}
void AncillaryTools::setGit(std::string&& inValue) noexcept
{
	m_git = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::hdiutil() const noexcept
{
	return m_hdiutil;
}
void AncillaryTools::setHdiutil(std::string&& inValue) noexcept
{
	m_hdiutil = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::installNameTool() const noexcept
{
	return m_installNameTool;
}
void AncillaryTools::setInstallNameTool(std::string&& inValue) noexcept
{
	m_installNameTool = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::instruments() const noexcept
{
	return m_instruments;
}
void AncillaryTools::setInstruments(std::string&& inValue) noexcept
{
	m_instruments = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::ldd() const noexcept
{
	return m_ldd;
}
void AncillaryTools::setLdd(std::string&& inValue) noexcept
{
	m_ldd = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::makeNsis() const noexcept
{
	return m_makeNsis;
}
void AncillaryTools::setMakeNsis(std::string&& inValue) noexcept
{
	m_makeNsis = std::move(inValue);
}

/*****************************************************************************/
const std::string AncillaryTools::osascript() const noexcept
{
	return m_osascript;
}

void AncillaryTools::setOsascript(std::string&& inValue) noexcept
{
	m_osascript = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::otool() const noexcept
{
	return m_otool;
}
void AncillaryTools::setOtool(std::string&& inValue) noexcept
{
	m_otool = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::plutil() const noexcept
{
	return m_plutil;
}
void AncillaryTools::setPlutil(std::string&& inValue) noexcept
{
	m_plutil = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::powershell() const noexcept
{
	return m_powershell;
}
void AncillaryTools::setPowershell(std::string&& inValue) noexcept
{
	m_powershell = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::sample() const noexcept
{
	return m_sample;
}
void AncillaryTools::setSample(std::string&& inValue) noexcept
{
	m_sample = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::sips() const noexcept
{
	return m_sips;
}
void AncillaryTools::setSips(std::string&& inValue) noexcept
{
	m_sips = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::tiffutil() const noexcept
{
	return m_tiffutil;
}
void AncillaryTools::setTiffutil(std::string&& inValue) noexcept
{
	m_tiffutil = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::xcodebuild() const noexcept
{
	return m_xcodebuild;
}
void AncillaryTools::setXcodebuild(std::string&& inValue) noexcept
{
	m_xcodebuild = std::move(inValue);
}
uint AncillaryTools::xcodeVersionMajor() const noexcept
{
	return m_xcodeVersionMajor;
}
uint AncillaryTools::xcodeVersionMinor() const noexcept
{
	return m_xcodeVersionMinor;
}

/*****************************************************************************/
const std::string& AncillaryTools::xcodegen() const noexcept
{
	return m_xcodegen;
}
void AncillaryTools::setXcodegen(std::string&& inValue) noexcept
{
	m_xcodegen = std::move(inValue);
}
uint AncillaryTools::xcodegenVersionMajor() const noexcept
{
	return m_xcodegenVersionMajor;
}
uint AncillaryTools::xcodegenVersionMinor() const noexcept
{
	return m_xcodegenVersionMinor;
}
uint AncillaryTools::xcodegenVersionPatch() const noexcept
{
	return m_xcodegenVersionPatch;
}

/*****************************************************************************/
const std::string& AncillaryTools::xcrun() const noexcept
{
	return m_xcrun;
}
void AncillaryTools::setXcrun(std::string&& inValue) noexcept
{
	m_xcrun = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::zip() const noexcept
{
	return m_zip;
}
void AncillaryTools::setZip(std::string&& inValue) noexcept
{
	m_zip = std::move(inValue);
}

/*****************************************************************************/
const std::string& AncillaryTools::vsperfcmd() const noexcept
{
	return m_vsperfcmd;
}
void AncillaryTools::setVsperfcmd(std::string&& inValue) noexcept
{
	m_vsperfcmd = std::move(inValue);
}

/*****************************************************************************/
bool AncillaryTools::macosCodeSignFile(const std::string& inPath, const bool inForce) const
{
#if defined(CHALET_MACOS)
	StringList cmd{ m_codesign, "--timestamp", "--options=runtime", "--strict", "--continue" };

	if (inForce)
		cmd.emplace_back("-f");

	cmd.emplace_back("-s");
	cmd.push_back(m_signingIdentity);

	if (Output::showCommands())
		cmd.emplace_back("-v");

	cmd.push_back(inPath);

	return Commands::subprocessNoOutput(cmd);
#else
	UNUSED(inPath, inForce);
	return false;
#endif
}

/*****************************************************************************/
bool AncillaryTools::macosCodeSignDiskImage(const std::string& inPath) const
{
#if defined(CHALET_MACOS)
	chalet_assert(String::endsWith(".dmg", inPath), "Must be a .dmg");

	StringList cmd{ m_codesign, "--timestamp", "--options=runtime", "--strict", "--continue", "-s", m_signingIdentity };

	if (Output::showCommands())
		cmd.emplace_back("-v");

	cmd.push_back(inPath);

	return Commands::subprocessNoOutput(cmd);
#else
	UNUSED(inPath);
	return false;
#endif
}

/*****************************************************************************/
bool AncillaryTools::macosCodeSignFileWithBundleVersion(const std::string& inFrameworkPath, const std::string& inVersionId) const
{
#if defined(CHALET_MACOS)
	chalet_assert(String::endsWith(".framework", inFrameworkPath), "Must be a .framework");

	StringList cmd{ m_codesign, "--timestamp", "--options=runtime", "--strict", "--continue", "-f", "-s", m_signingIdentity };
	cmd.emplace_back(fmt::format("-bundle-version={}", inVersionId));

	if (Output::showCommands())
		cmd.emplace_back("-v");

	cmd.push_back(inFrameworkPath);
	return Commands::subprocessNoOutput(cmd);
#else
	UNUSED(inFrameworkPath, inVersionId);
	return false;
#endif
}

/*****************************************************************************/
bool AncillaryTools::plistConvertToBinary(const std::string& inInput, const std::string& inOutput) const
{
#if defined(CHALET_MACOS)
	return Commands::subprocess({ m_plutil, "-convert", "binary1", inInput, "-o", inOutput });
#else
	UNUSED(inInput, inOutput);
	return false;
#endif
}

/*****************************************************************************/
bool AncillaryTools::plistConvertToJson(const std::string& inInput, const std::string& inOutput) const
{
#if defined(CHALET_MACOS)
	return Commands::subprocess({ m_plutil, "-convert", "json", inInput, "-o", inOutput });
#else
	UNUSED(inInput, inOutput);
	return false;
#endif
}

/*****************************************************************************/
std::string AncillaryTools::getPathToGit()
{
	auto git = Commands::which("git");
#if defined(CHALET_WIN32)
	if (git.empty())
	{
		auto programs = Environment::getAsString("ProgramFiles");
		if (!programs.empty())
		{
			auto gitPath = fmt::format("{}/Git/bin/git.exe", programs);
			Path::sanitize(gitPath);
			if (Commands::pathExists(gitPath))
			{
				git = std::move(gitPath);
			}
		}
	}
#endif

	return git;
}

}
