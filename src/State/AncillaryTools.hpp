/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/ScriptAdapter.hpp"
#include "State/VariableAdapter.hpp"

namespace chalet
{
struct MacosCodeSignOptions;

struct AncillaryTools
{
	AncillaryTools();

	bool validate(const std::string& inHomeDirectory);

	void fetchBashVersion();
	void fetchXcodeVersion();

	const ScriptAdapter& scriptAdapter() const;

	std::string getApplePlatformSdk(const std::string& inKey) const;
	void addApplePlatformSdk(const std::string& inKey, std::string&& inValue);

	const std::string& bash() const noexcept;
	void setBash(std::string&& inValue) noexcept;
	bool bashAvailable() const noexcept;

	const std::string& ccache() const noexcept;
	void setCCache(std::string&& inValue) noexcept;

	const std::string& codesign() const noexcept;
	void setCodesign(std::string&& inValue) noexcept;

	const std::string& signingIdentity() const noexcept;
	const std::string& signingDevelopmentTeam() const noexcept;
	const std::string& signingCertificate() const noexcept;
	void setSigningIdentity(const std::string& inValue) noexcept;
	bool isSigningIdentityValid() const;

	const std::string& commandPrompt() const noexcept;
	void setCommandPrompt(std::string&& inValue) noexcept;

	const std::string& git() const noexcept;
	void setGit(std::string&& inValue) noexcept;

	const std::string& hdiutil() const noexcept;
	void setHdiutil(std::string&& inValue) noexcept;

	const std::string& installNameTool() const noexcept;
	void setInstallNameTool(std::string&& inValue) noexcept;

	const std::string& instruments() const noexcept;
	void setInstruments(std::string&& inValue) noexcept;

	const std::string& ldd() const noexcept;
	void setLdd(std::string&& inValue) noexcept;

	const std::string osascript() const noexcept;
	void setOsascript(std::string&& inValue) noexcept;

	const std::string& otool() const noexcept;
	void setOtool(std::string&& inValue) noexcept;

	const std::string& plutil() const noexcept;
	void setPlutil(std::string&& inValue) noexcept;

	const std::string& powershell() const noexcept;
	void setPowershell(std::string&& inValue) noexcept;

	const std::string& sample() const noexcept;
	void setSample(std::string&& inValue) noexcept;

	const std::string& sips() const noexcept;
	void setSips(std::string&& inValue) noexcept;

	const std::string& tar() const noexcept;
	void setTar(std::string&& inValue) noexcept;

	const std::string& tiffutil() const noexcept;
	void setTiffutil(std::string&& inValue) noexcept;

	const std::string& xcodebuild() const noexcept;
	void setXcodebuild(std::string&& inValue) noexcept;
	u32 xcodeVersionMajor() const noexcept;
	u32 xcodeVersionMinor() const noexcept;

	const std::string& xcrun() const noexcept;
	void setXcrun(std::string&& inValue) noexcept;

	const std::string& zip() const noexcept;
	void setZip(std::string&& inValue) noexcept;

	// NOT cached
	const std::string& vsperfcmd() const noexcept;
	void setVsperfcmd(std::string&& inValue) noexcept;

	// Commands

	bool macosCodeSignFile(const std::string& inPath, const MacosCodeSignOptions& inOptions) const;
	bool macosCodeSignDiskImage(const std::string& inPath, const MacosCodeSignOptions& inOptions) const;
	bool macosCodeSignFileWithBundleVersion(const std::string& inFrameworkPath, const std::string& inVersionId, const MacosCodeSignOptions& inOptions) const;

	bool plistConvertToBinary(const std::string& inInput, const std::string& inOutput) const;
	bool plistConvertToJson(const std::string& inInput, const std::string& inOutput) const;
	bool plistConvertToXml(const std::string& inInput, const std::string& inOutput) const;
	bool plistCreateNew(const std::string& inOutput) const;

	static std::string getPathToGit();
	static bool gitIsRootPath(std::string& outPath);

	VariableAdapter variables;

private:
	ScriptAdapter m_scriptAdapter;

	Dictionary<std::string> m_applePlatformSdk;

	std::string m_chalet;

	std::string m_bash;
	std::string m_ccache;
	std::string m_codesign;
	std::string m_commandPrompt;
	std::string m_git;
	std::string m_gprof;
	std::string m_hdiutil;
	std::string m_installNameTool;
	std::string m_instruments;
	std::string m_ldd;
	std::string m_lipo;
	std::string m_lua;
	std::string m_osascript;
	std::string m_otool;
	std::string m_perl;
	std::string m_plutil;
	std::string m_powershell;
	std::string m_python;
	std::string m_python3;
	std::string m_ruby;
	std::string m_sample;
	std::string m_sips;
	std::string m_tar;
	std::string m_tiffutil;
	std::string m_xcodebuild;
	std::string m_xcrun;
	std::string m_zip;

	std::string m_vsperfcmd;

	std::string m_signingIdentity;
#if defined(CHALET_MACOS)
	mutable std::string m_signingDevelopmentTeam;
	mutable std::string m_signingCertificate;
#endif

	u32 m_xcodeVersionMajor = 0;
	u32 m_xcodeVersionMinor = 0;

	bool m_bashAvailable = false;
};
}
