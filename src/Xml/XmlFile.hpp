/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_XML_FILE_HPP
#define CHALET_XML_FILE_HPP

#include "Xml/Xml.hpp"

namespace chalet
{
struct XmlFile
{
	XmlFile() = default;
	explicit XmlFile(std::string inFilename);

	static bool saveToFile(const Xml& inXml, const std::string& outFilename, const int inIndent = 1);

	bool save(const int inIndent = 1);

	bool dirty() const noexcept;
	void setDirty(const bool inValue) noexcept;

	void resetAndSave();
	void dumpToTerminal();

	void setContents(Xml&& inXml);

	const std::string& filename() const noexcept;

	Xml xml;

private:
	std::string m_filename;

	bool m_dirty = false;
};
}

#endif // CHALET_XML_FILE_HPP