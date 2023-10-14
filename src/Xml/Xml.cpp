/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Xml/Xml.hpp"

namespace chalet
{
/*****************************************************************************/
Xml::Xml(std::string inRootName) :
	m_root(std::move(inRootName))
{
}

/*****************************************************************************/
std::string Xml::dump(const int inIndent, const char inIndentChar) const
{
	std::string ret;
	if (m_standalone)
		ret = fmt::format("<?xml version=\"{}\" encoding=\"{}\" standalone=\"yes\" ?>", m_version, m_encoding);
	else
		ret = fmt::format("<?xml version=\"{}\" encoding=\"{}\" ?>", m_version, m_encoding);

	if (inIndent >= 0)
		ret += '\n';

	for (const auto& header : m_headers)
	{
		ret += header;
		ret += '\n';
	}

	ret += m_root.dump(0, inIndent, inIndentChar);
	if (ret.back() == '\n')
		ret.pop_back();

	return ret;
}

/*****************************************************************************/
void Xml::addRawHeader(std::string inHeader)
{
	while (inHeader.back() == '\n')
		inHeader.pop_back();

	m_headers.emplace_back(std::move(inHeader));
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
bool Xml::standalone() const noexcept
{
	return m_standalone;
}
void Xml::setStandalone(const bool inValue) noexcept
{
	m_standalone = inValue;
}

/*****************************************************************************/
XmlElement& Xml::root()
{
	return m_root;
}
}
