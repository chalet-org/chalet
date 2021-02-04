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

	const std::string& ar() const noexcept;
	void setAr(const std::string& inValue) noexcept;

	const std::string& brew() const noexcept;
	void setBrew(const std::string& inValue) noexcept;

	const std::string& cmake() const noexcept;
	void setCmake(const std::string& inValue) noexcept;

	const std::string& codeSign() const noexcept;
	void setCodeSign(const std::string& inValue) noexcept;

	const std::string& git() const noexcept;
	void setGit(const std::string& inValue) noexcept;

	const std::string& hdiUtil() const noexcept;
	void setHdiUtil(const std::string& inValue) noexcept;

	const std::string& installNameUtil() const noexcept;
	void setInstallNameUtil(const std::string& inValue) noexcept;

	const std::string& ldd() const noexcept;
	void setLdd(const std::string& inValue) noexcept;

	const std::string& macosSdk() const noexcept;
	void setMacosSdk(const std::string& inValue) noexcept;

	const std::string& make() const noexcept;
	void setMake(const std::string& inValue) noexcept;
	uint makeVersionMajor() const noexcept;
	uint makeVersionMinor() const noexcept;

	const std::string& sips() const noexcept;
	void setSips(const std::string& inValue) noexcept;

	const std::string& ninja() const noexcept;
	void setNinja(const std::string& inValue) noexcept;

	const std::string& otool() const noexcept;
	void setOtool(const std::string& inValue) noexcept;

	const std::string& plUtil() const noexcept;
	void setPlUtil(const std::string& inValue) noexcept;

	const std::string& ranlib() const noexcept;
	void setRanlib(const std::string& inValue) noexcept;

	const std::string& strip() const noexcept;
	void setStrip(const std::string& inValue) noexcept;

	const std::string& tiffUtil() const noexcept;
	void setTiffUtil(const std::string& inValue) noexcept;

	// Commands
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
	std::string m_ar;
	std::string m_brew;
	std::string m_cmake;
	std::string m_codeSign;
	std::string m_git;
	std::string m_hdiUtil;
	std::string m_installNameUtil;
	std::string m_ldd;
	std::string m_macosSdk;
	std::string m_make;
	std::string m_ninja;
	std::string m_otool;
	std::string m_plUtil;
	std::string m_ranlib;
	std::string m_sips;
	std::string m_strip;
	std::string m_tiffUtil;

	uint m_makeVersionMajor;
	uint m_makeVersionMinor;
};
}

#endif // CHALET_CACHE_TOOLS_HPP
