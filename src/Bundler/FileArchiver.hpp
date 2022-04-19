/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_FILE_ARCHIVER_HPP
#define CHALET_FILE_ARCHIVER_HPP

#include "State/ArchiveFormat.hpp"

namespace chalet
{
class BuildState;
struct BundleArchiveTarget;

struct FileArchiver
{
	explicit FileArchiver(const BuildState& inState);

	bool archive(const BundleArchiveTarget& inTarget, const std::string& inBaseName, const StringList& inFiles, const StringList& inExcludes);

private:
	bool powerShellIsValid() const;
	bool zipIsValid() const;
	bool tarIsValid() const;

	std::string makeTemporaryDirectory(const std::string& inBaseName, const StringList& inFiles, const StringList& inExcludes) const;

	StringList getZipFormatCommand(const std::string& inBaseName, const StringList& inFiles) const;
	StringList getTarFormatCommand(const std::string& inBaseName, const StringList& inFiles) const;

	const BuildState& m_state;
	const std::string& m_outputDirectory;

	std::string m_outputFilename;
	std::string m_tmpDirectory;

	ArchiveFormat m_format = ArchiveFormat::Zip;
};
}

#endif // CHALET_FILE_ARCHIVER_HPP
