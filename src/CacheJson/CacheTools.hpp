/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CACHE_TOOLS_HPP
#define CHALET_CACHE_TOOLS_HPP

namespace chalet
{
struct CacheTools
{
	CacheTools() = default;

	void fetchVersions();

	const std::string& ar() const noexcept;
	void setAr(const std::string& inValue) noexcept;

	const std::string& bash() const noexcept;
	void setBash(const std::string& inValue) noexcept;
	bool bashAvailable() const noexcept;

	const std::string& brew() const noexcept;
	void setBrew(const std::string& inValue) noexcept;
	bool brewAvailable() noexcept;

	const std::string& cmake() const noexcept;
	void setCmake(const std::string& inValue) noexcept;
	uint cmakeVersionMajor() const noexcept;
	uint cmakeVersionMinor() const noexcept;
	uint cmakeVersionPatch() const noexcept;
	bool cmakeAvailable() const noexcept;

	const std::string& codesign() const noexcept;
	void setCodesign(const std::string& inValue) noexcept;

	const std::string& commandPrompt() const noexcept;
	void setCommandPrompt(const std::string& inValue) noexcept;

	const std::string& git() const noexcept;
	void setGit(const std::string& inValue) noexcept;

	const std::string& gprof() const noexcept;
	void setGprof(const std::string& inValue) noexcept;

	const std::string& hdiutil() const noexcept;
	void setHdiutil(const std::string& inValue) noexcept;

	const std::string& installNameUtil() const noexcept;
	void setInstallNameUtil(const std::string& inValue) noexcept;

	const std::string& instruments() const noexcept;
	void setInstruments(const std::string& inValue) noexcept;

	const std::string& ldd() const noexcept;
	void setLdd(const std::string& inValue) noexcept;

	const std::string& libtool() const noexcept;
	void setLibtool(const std::string& inValue) noexcept;

	const std::string& lua() const noexcept;
	void setLua(const std::string& inValue) noexcept;

	const std::string& macosSdk() const noexcept;
	void setMacosSdk(const std::string& inValue) noexcept;

	const std::string& make() const noexcept;
	void setMake(const std::string& inValue) noexcept;
	uint makeVersionMajor() const noexcept;
	uint makeVersionMinor() const noexcept;
	bool isNMake() const noexcept;

	const std::string& ninja() const noexcept;
	void setNinja(const std::string& inValue) noexcept;

	const std::string& objdump() const noexcept;
	void setObjdump(const std::string& inValue) noexcept;

	const std::string osascript() const noexcept;
	void setOsascript(const std::string& inValue) noexcept;

	const std::string& otool() const noexcept;
	void setOtool(const std::string& inValue) noexcept;

	const std::string& perl() const noexcept;
	void setPerl(const std::string& inValue) noexcept;

	const std::string& plutil() const noexcept;
	void setPlutil(const std::string& inValue) noexcept;

	const std::string& powershell() const noexcept;
	void setPowershell(const std::string& inValue) noexcept;

	const std::string& python() const noexcept;
	void setPython(const std::string& inValue) noexcept;

	const std::string& python3() const noexcept;
	void setPython3(const std::string& inValue) noexcept;

	const std::string& ranlib() const noexcept;
	void setRanlib(const std::string& inValue) noexcept;

	const std::string& ruby() const noexcept;
	void setRuby(const std::string& inValue) noexcept;

	const std::string& sample() const noexcept;
	void setSample(const std::string& inValue) noexcept;

	const std::string& sips() const noexcept;
	void setSips(const std::string& inValue) noexcept;

	const std::string& strip() const noexcept;
	void setStrip(const std::string& inValue) noexcept;

	const std::string& tiffutil() const noexcept;
	void setTiffutil(const std::string& inValue) noexcept;

	const std::string& xcodebuild() const noexcept;
	void setXcodebuild(const std::string& inValue) noexcept;
	uint xcodeVersionMajor() const noexcept;
	uint xcodeVersionMinor() const noexcept;

	// xcodegen
	const std::string& xcodegen() const noexcept;
	void setXcodegen(const std::string& inValue) noexcept;
	uint xcodegenVersionMajor() const noexcept;
	uint xcodegenVersionMinor() const noexcept;
	uint xcodegenVersionPatch() const noexcept;

	const std::string& xcrun() const noexcept;
	void setXcrun(const std::string& inValue) noexcept;

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
	void isolateVersion(std::string& outString);

	std::string m_ar;
	std::string m_bash;
	std::string m_brew;
	std::string m_cmake;
	std::string m_codesign;
	std::string m_commandPrompt;
	std::string m_git;
	std::string m_gprof;
	std::string m_hdiutil;
	std::string m_installNameUtil;
	std::string m_instruments;
	std::string m_ldd;
	std::string m_libtool;
	std::string m_lua;
	std::string m_macosSdk;
	std::string m_make;
	std::string m_ninja;
	std::string m_objdump;
	std::string m_osascript;
	std::string m_otool;
	std::string m_perl;
	std::string m_plutil;
	std::string m_powershell;
	std::string m_python;
	std::string m_python3;
	std::string m_ranlib;
	std::string m_ruby;
	std::string m_sample;
	std::string m_sips;
	std::string m_strip;
	std::string m_tiffutil;
	std::string m_xcodebuild;
	std::string m_xcodegen;
	std::string m_xcrun;

	uint m_cmakeVersionMajor = 0;
	uint m_cmakeVersionMinor = 0;
	uint m_cmakeVersionPatch = 0;

	uint m_makeVersionMajor = 0;
	uint m_makeVersionMinor = 0;

	uint m_xcodeVersionMajor = 0;
	uint m_xcodeVersionMinor = 0;

	uint m_xcodegenVersionMajor = 0;
	uint m_xcodegenVersionMinor = 0;
	uint m_xcodegenVersionPatch = 0;

	bool m_brewAvailable = false;
	bool m_bashAvailable = false;
	bool m_cmakeAvailable = false;
	bool m_makeIsNMake = false;
};
}

#endif // CHALET_CACHE_TOOLS_HPP
