/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ZIP_ARCHIVER_HPP
#define CHALET_ZIP_ARCHIVER_HPP

struct zip_t;

namespace chalet
{
struct ZipArchiver
{
	ZipArchiver() = default;
	CHALET_DISALLOW_COPY_MOVE(ZipArchiver);
	~ZipArchiver();

	bool openArchiveForWriting(std::string inFilename);

	bool addToArchive(const std::string& inPath);

private:
	std::string m_filename;
	std::string m_cwd;

	::zip_t* m_zip = nullptr;
};
}

#endif // CHALET_ZIP_ARCHIVER_HPP
