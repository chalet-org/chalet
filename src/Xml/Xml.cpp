/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Xml/Xml.hpp"

namespace chalet
{
/*****************************************************************************/
Xml::Xml(std::string inRootName) :
	m_version("1.0"),
	m_encoding("utf-8"),
	m_root(std::make_unique<XmlNode>(std::move(inRootName)))
{
}

/*****************************************************************************/
std::string Xml::toString() const
{
	std::string ret = fmt::format("<?xml version=\"{}\" encoding=\"{}\"?>\n", m_version, m_encoding);
	ret += m_root->toString();

	return ret;
}

/*****************************************************************************/
const std::string& Xml::version() const noexcept
{
	return m_version;
}

void Xml::setVersion(const std::string& inVersion)
{
	m_version = inVersion;
}

/*****************************************************************************/
const std::string& Xml::encoding() const noexcept
{
	return m_encoding;
}

void Xml::setEncoding(const std::string& inVersion)
{
	m_encoding = inVersion;
}

/*****************************************************************************/
XmlNode& Xml::root()
{
	return *m_root;
}
}
