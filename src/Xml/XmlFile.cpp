/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Xml/XmlFile.hpp"

#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
XmlFile::XmlFile(std::string inFilename) :
	m_filename(std::move(inFilename))
{
}

/*****************************************************************************/
bool XmlFile::saveToFile(const Xml& inXml, const std::string& outFilename, const int inIndent)
{
	if (outFilename.empty())
		return false;

	const auto folder = String::getPathFolder(outFilename);
	if (!folder.empty() && !Commands::pathExists(folder))
	{
		if (!Commands::makeDirectory(folder))
			return false;
	}

	if (inIndent <= 0 || inIndent > 4)
		std::ofstream(outFilename) << inXml.dump() << std::endl;
	else
		std::ofstream(outFilename) << inXml.dump(inIndent, '\t') << std::endl;

	return true;
}

/*****************************************************************************/
bool XmlFile::save(const int inIndent)
{
	if (!m_filename.empty() && m_dirty)
	{
		return XmlFile::saveToFile(xml, m_filename, inIndent);
	}

	// if there's nothing to save, we don't care
	return true;
}

/*****************************************************************************/
bool XmlFile::dirty() const noexcept
{
	return m_dirty;
}

void XmlFile::setDirty(const bool inValue) noexcept
{
	m_dirty = inValue;
}

/*****************************************************************************/
void XmlFile::resetAndSave()
{
	xml = Xml();
	setDirty(true);
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
}