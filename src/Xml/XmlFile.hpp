/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Xml/Xml.hpp"

namespace chalet
{
struct XmlFile
{
	XmlFile() = delete;
	explicit XmlFile(std::string inFilename);

	static bool saveToFile(const Xml& inXml, const std::string& outFilename, const i32 inIndent = 2);

	bool save(const i32 inIndent = 2);

	void resetAndSave();
	void dumpToTerminal();

	void setContents(Xml&& inXml);

	const std::string& filename() const noexcept;

	XmlElement& getRoot();

	Xml xml;

private:
	std::string m_filename;
};
}
