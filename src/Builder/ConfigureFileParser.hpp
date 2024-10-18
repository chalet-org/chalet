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
	void replaceEmbeddableFiles(std::string& outContent);
	void replaceEmbeddable(std::string& outContent, const std::string& inKeyword, const std::function<bool(std::string&, const std::string&)>& inGenerator);

	bool generateBytesForFile(std::string& outText, const std::string& inFile) const;
	// bool generateCharsForFile(std::string& outText, const std::string& inFile) const;
	// bool generateStringForFile(std::string& outText, const std::string& inFile) const;

	const BuildState& m_state;
	const SourceTarget& m_project;

	Dictionary<std::string> m_embeddedFiles;
	std::string m_cacheFile;
	std::string m_currentFile;

	bool m_failure = false;
};
}
