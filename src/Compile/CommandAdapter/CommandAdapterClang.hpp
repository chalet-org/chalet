/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;
struct SourceTarget;

struct CommandAdapterClang
{
	explicit CommandAdapterClang(const BuildState& inState, const SourceTarget& inProject);

	std::string getLanguageStandardCpp() const;
	std::string getLanguageStandardC() const;
	std::string getCxxLibrary() const;
	std::string getOptimizationLevel() const;

	StringList getWarningList() const;
	StringList getWarningExclusions() const;
	StringList getSanitizersList() const;

	// StringList getIncludeDirectories() const;
	// StringList getAdditionalCompilerOptions(const bool inCharsetFlags = false) const;
	// StringList getAdditionalLinkerOptions() const;

	// StringList getLibDirectories() const;
	// StringList getLinks(const bool inIncludeCore = true) const;

	// bool createPrecompiledHeaderSource(const std::string& inSourcePath, const std::string& inPchPath);
	// const std::string& pchSource() const noexcept;
	// const std::string& pchTarget() const noexcept;
	// const std::string& pchMinusLocation() const noexcept;

	bool supportsCppCoroutines() const;
	bool supportsCppConcepts() const;
	bool supportsFastMath() const;
	bool supportsExceptions() const;
	bool supportsRunTimeTypeInformation() const;

private:
	const BuildState& m_state;
	const SourceTarget& m_project;

	// std::string m_pchSource;
	// std::string m_pchTarget;
	// std::string m_pchMinusLocation;

	uint m_versionMajorMinor = 0;
	uint m_versionPatch = 0;
};
}
