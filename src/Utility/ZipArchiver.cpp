/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/ZipArchiver.hpp"

#include "Libraries/Zip.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool ZipArchiver::openArchiveForWriting(std::string inFilename)
{
	m_filename = std::move(inFilename);
	m_cwd = Commands::getWorkingDirectory();

	m_zip = zip_open(m_filename.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

	return m_zip != nullptr;
}

/*****************************************************************************/
bool ZipArchiver::addToArchive(const std::string& inPath)
{
	if (m_zip == nullptr || m_filename.empty())
		return false;

	if (zip_entry_open(m_zip, "foo-2.txt") < 0)
		return false;

	bool writeResult = zip_entry_fwrite(m_zip, inPath.c_str()) < 0;

	if (zip_entry_close(m_zip) < 0)
		return false;

	return writeResult;
}

/*****************************************************************************/
ZipArchiver::~ZipArchiver()
{
	if (m_zip != nullptr)
	{
		zip_close(m_zip);
		m_zip = nullptr;
	}
}
}
