/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ANCILLARY_TOOLS_HPP
#define CHALET_ANCILLARY_TOOLS_HPP

namespace chalet
{
struct AncillaryTools
{
	AncillaryTools() = default;

	bool resolveOwnExecutable(const std::string& inAppPath);

	void fetchBashVersion();
	void fetchBrewVersion();
	void fetchXcodeVersion();
	void fetchXcodeGenVersion();

	const std::string& chalet() const noexcept;

	const std::string& bash() const noexcept;
	void setBash(std::string&& inValue) noexcept;
	bool bashAvailable() const noexcept;

	const std::string& brew() const noexcept;
	void setBrew(std::string&& inValue) noexcept;
	bool brewAvailable() const noexcept;

	const std::string& codesign() const noexcept;
	void setCodesign(std::string&& inValue) noexcept;

	const std::string& commandPrompt() const noexcept;
	void setCommandPrompt(std::string&& inValue) noexcept;

	const std::string& git() const noexcept;
	void setGit(std::string&& inValue) noexcept;

	const std::string& gprof() const noexcept;
	void setGprof(std::string&& inValue) noexcept;

	const std::string& hdiutil() const noexcept;
	void setHdiutil(std::string&& inValue) noexcept;

	const std::string& installNameTool() const noexcept;
	void setInstallNameTool(std::string&& inValue) noexcept;

	const std::string& instruments() const noexcept;
	void setInstruments(std::string&& inValue) noexcept;

	const std::string& ldd() const noexcept;
	void setLdd(std::string&& inValue) noexcept;

	const std::string& lipo() const noexcept;
	void setLipo(std::string&& inValue) noexcept;

	const std::string& lua() const noexcept;
	void setLua(std::string&& inValue) noexcept;

	const std::string& applePlatformSdk(const std::string& inKey) const;
	void addApplePlatformSdk(const std::string& inKey, std::string&& inValue);

	const std::string osascript() const noexcept;
	void setOsascript(std::string&& inValue) noexcept;

	const std::string& otool() const noexcept;
	void setOtool(std::string&& inValue) noexcept;

	const std::string& perl() const noexcept;
	void setPerl(std::string&& inValue) noexcept;

	const std::string& plutil() const noexcept;
	void setPlutil(std::string&& inValue) noexcept;

	const std::string& powershell() const noexcept;
	void setPowershell(std::string&& inValue) noexcept;

	const std::string& python() const noexcept;
	void setPython(std::string&& inValue) noexcept;

	const std::string& python3() const noexcept;
	void setPython3(std::string&& inValue) noexcept;

	const std::string& ruby() const noexcept;
	void setRuby(std::string&& inValue) noexcept;

	const std::string& sample() const noexcept;
	void setSample(std::string&& inValue) noexcept;

	const std::string& sips() const noexcept;
	void setSips(std::string&& inValue) noexcept;

	const std::string& tiffutil() const noexcept;
	void setTiffutil(std::string&& inValue) noexcept;

	const std::string& xcodebuild() const noexcept;
	void setXcodebuild(std::string&& inValue) noexcept;
	uint xcodeVersionMajor() const noexcept;
	uint xcodeVersionMinor() const noexcept;

	// xcodegen
	const std::string& xcodegen() const noexcept;
	void setXcodegen(std::string&& inValue) noexcept;
	uint xcodegenVersionMajor() const noexcept;
	uint xcodegenVersionMinor() const noexcept;
	uint xcodegenVersionPatch() const noexcept;

	const std::string& xcrun() const noexcept;
	void setXcrun(std::string&& inValue) noexcept;

	// Commands
	std::string getAsmGenerateCommand(const std::string& inputFile, const std::string& outputFile) const;

	bool installHomebrewPackage(const std::string& inPackage, const bool inCleanOutput = true) const;

	std::string getCurrentGitRepositoryBranch(const std::string& inRepoPath, const bool inCleanOutput = true) const;
	std::string getCurrentGitRepositoryTag(const std::string& inRepoPath, const bool inCleanOutput = true) const;
	std::string getCurrentGitRepositoryHash(const std::string& inRepoPath, const bool inCleanOutput = true) const;
	std::string getCurrentGitRepositoryHashFromRemote(const std::string& inRepoPath, const std::string& inBranch, const bool inCleanOutput = true) const;
	bool updateGitRepositoryShallow(const std::string& inRepoPath, const bool inCleanOutput = true) const;
	bool resetGitRepositoryToCommit(const std::string& inRepoPath, const std::string& inCommit, const bool inCleanOutput = true) const;

	bool plistConvertToBinary(const std::string& inInput, const std::string& inOutput, const bool inCleanOutput = true) const;
	bool plistReplaceProperty(const std::string& inPlistFile, const std::string& inKey, const std::string& inValue, const bool inCleanOutput = true) const;
	bool getExecutableDependencies(const std::string& inPath, StringList& outList) const;

private:
	std::unordered_map<std::string, std::string> m_applePlatformSdk;

	std::string m_chalet;

	std::string m_bash;
	std::string m_brew;
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
	std::string m_tiffutil;
	std::string m_xcodebuild;
	std::string m_xcodegen;
	std::string m_xcrun;

	uint m_xcodeVersionMajor = 0;
	uint m_xcodeVersionMinor = 0;

	uint m_xcodegenVersionMajor = 0;
	uint m_xcodegenVersionMinor = 0;
	uint m_xcodegenVersionPatch = 0;

	bool m_brewAvailable = false;
	bool m_bashAvailable = false;
};
}

#endif // CHALET_ANCILLARY_TOOLS_HPP
