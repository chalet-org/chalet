/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;
struct SourceTarget;
struct IBuildTarget;

struct ConfigureFileParser
{
	explicit ConfigureFileParser(const BuildState& inState, const SourceTarget& inProject);

	bool run(const std::string& inOutputFolder);

private:
	std::string getReplaceValue(std::string inKey) const;
	std::string getReplaceValueFromSubString(const std::string& inKey, const bool isTarget = false) const;
	void replaceEmbeddable(std::string& outContent, StringList& outCache);

	bool generateBytesForFile(std::string& outText, const std::string& inFile) const;

	const BuildState& m_state;
	const SourceTarget& m_project;

	StringList m_embedFileCache;
	std::string m_cacheFile;

	bool m_failure = false;
};
}
