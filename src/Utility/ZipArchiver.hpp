/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ZIP_ARCHIVER_HPP
#define CHALET_ZIP_ARCHIVER_HPP

namespace chalet
{
struct StatePrototype;

struct ZipArchiver
{
	explicit ZipArchiver(const StatePrototype& inPrototype);

	bool archive(const std::string& inFilename, const StringList& inFiles, const std::string& inCwd);

private:
	const StatePrototype& m_prototype;
};
}

#endif // CHALET_ZIP_ARCHIVER_HPP
