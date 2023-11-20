/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;
struct BundleArchiveTarget;

struct FileArchiver
{
	explicit FileArchiver(const BuildState& inState);

	bool archive(const BundleArchiveTarget& inTarget, const std::string& inBaseName, const StringList& inIncludes, const StringList& inExcludes);

	bool notarize(const BundleArchiveTarget& inTarget);

private:
	bool powerShellIsValid() const;
	bool zipIsValid() const;
	bool tarIsValid() const;

	StringList getResolvedIncludes(const StringList& inIncludes) const;
	std::string makeTemporaryDirectory(const std::string& inBaseName) const;
	bool copyIncludestoTemporaryDirectory(const StringList& inIncludes, const StringList& inExcludes, const std::string& inDirectory) const;

	bool getZipFormatCommand(StringList& outCmd, const std::string& inBaseName, const StringList& inIncludes) const;
	bool getTarFormatCommand(StringList& outCmd, const std::string& inBaseName, const StringList& inIncludes) const;
	StringList getIncludesForCommand(const StringList& inIncludes) const;

	const BuildState& m_state;
	const std::string& m_outputDirectory;

	std::string m_outputFilename;
	std::string m_tmpDirectory;
};
}
