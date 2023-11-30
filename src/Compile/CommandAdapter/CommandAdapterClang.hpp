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

	bool supportsCppCoroutines() const;
	bool supportsCppConcepts() const;
	bool supportsFastMath() const;
	bool supportsExceptions() const;
	bool supportsRunTimeTypeInformation() const;

private:
	const BuildState& m_state;
	const SourceTarget& m_project;

	u32 m_versionMajorMinor = 0;
	u32 m_versionPatch = 0;
};
}
