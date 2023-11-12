/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Xml/XmlFile.hpp"

#include "System/Files.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
XmlFile::XmlFile(std::string inFilename) :
	m_filename(std::move(inFilename))
{
}

/*****************************************************************************/
bool XmlFile::saveToFile(const Xml& inXml, const std::string& outFilename, const i32 inIndent)
{
	if (outFilename.empty())
		return false;

	const auto folder = String::getPathFolder(outFilename);
	if (!folder.empty() && !Files::pathExists(folder))
	{
		if (!Files::makeDirectory(folder))
			return false;
	}

	if ((inIndent < -1 || inIndent > 4) || inIndent == 1)
		std::ofstream(outFilename) << inXml.dump(1, '\t') << std::endl;
	else
		std::ofstream(outFilename) << inXml.dump(inIndent) << std::endl;

	return true;
}

/*****************************************************************************/
bool XmlFile::save(const i32 inIndent)
{
	if (!m_filename.empty())
	{
		return XmlFile::saveToFile(xml, m_filename, inIndent);
	}

	// if there's nothing to save, we don't care
	return true;
}

/*****************************************************************************/
void XmlFile::resetAndSave()
{
	xml = Xml();
	save();
}

/*****************************************************************************/
void XmlFile::dumpToTerminal()
{
	auto output = xml.dump(1, '\t');
	std::cout.write(output.data(), output.size());
	std::cout.put('\n');
	std::cout.flush();
}

/*****************************************************************************/
void XmlFile::setContents(Xml&& inXml)
{
	xml = std::move(inXml);
}

/*****************************************************************************/
const std::string& XmlFile::filename() const noexcept
{
	return m_filename;
}

/*****************************************************************************/
XmlElement& XmlFile::getRoot()
{
	return xml.root();
}
}
