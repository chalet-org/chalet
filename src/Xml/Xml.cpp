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
	m_root(std::move(inRootName))
{
}

/*****************************************************************************/
std::string Xml::dump(const int inIndent, const char inIndentChar) const
{
	std::string ret = fmt::format("<?xml version=\"{}\" encoding=\"{}\"?>", m_version, m_encoding);
	if (inIndent >= 0)
		ret += '\n';

	ret += m_root.dump(0, inIndent, inIndentChar);
	if (ret.back() == '\n')
		ret.pop_back();

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
	return m_root;
}
}
