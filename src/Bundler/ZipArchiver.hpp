/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ZIP_ARCHIVER_HPP
#define CHALET_ZIP_ARCHIVER_HPP

namespace chalet
{
struct CentralState;

struct ZipArchiver
{
	explicit ZipArchiver(const CentralState& inCentralState);

	bool archive(const std::string& inFilename, const StringList& inFiles, const std::string& inCwd, const StringList& inExcludes);

private:
	const CentralState& m_centralState;
};
}

#endif // CHALET_ZIP_ARCHIVER_HPP
