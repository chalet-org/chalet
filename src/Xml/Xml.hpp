/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_XML_HPP
#define CHALET_XML_HPP

#include "Xml/XmlNode.hpp"

namespace chalet
{
struct Xml
{
	explicit Xml(std::string inRootName);

	std::string toString() const;

	const std::string& version() const noexcept;
	void setVersion(const std::string& inVersion);

	const std::string& encoding() const noexcept;
	void setEncoding(const std::string& inVersion);

	XmlNode& root();

private:
	std::string m_version;
	std::string m_encoding;

	Unique<XmlNode> m_root;
};
}

#endif // CHALET_XML_HPP
