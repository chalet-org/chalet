/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Xml/XmlNode.hpp"

namespace chalet
{
/*****************************************************************************/
XmlNode::XmlNode(std::string inName) :
	m_name(std::move(inName))
{
}

/*****************************************************************************/
const std::string& XmlNode::name() const noexcept
{
	return m_name;
}

/*****************************************************************************/
bool XmlNode::hasAttribute(const std::string& inKey) const
{
	if (m_attributes == nullptr)
		return false;

	if (m_attributes->find(inKey) == m_attributes->end())
		return false;

	return true;
}

/*****************************************************************************/
const std::string& XmlNode::getAttribute(const std::string& inKey) const
{
	return m_attributes->at(inKey);
}

/*****************************************************************************/
bool XmlNode::addAttribute(const std::string& inKey, std::string inValue)
{
	makeAttributes();

	if (hasAttribute(inKey))
		return false;

	m_attributes->emplace(inKey, std::move(inValue));
	return true;
}

/*****************************************************************************/
bool XmlNode::setAttribute(const std::string& inKey, std::string inValue)
{
	makeAttributes();

	(*m_attributes)[inKey] = std::move(inValue);
	return true;
}

/*****************************************************************************/
bool XmlNode::removeAttribute(const std::string& inKey)
{
	if (!hasAttribute(inKey))
		return false;

	m_attributes->erase(inKey);
	return true;
}

/*****************************************************************************/
bool XmlNode::clearAttributes()
{
	if (m_attributes != nullptr)
	{
		m_attributes.reset();
		return true;
	}

	return false;
}

/*****************************************************************************/
bool XmlNode::hasChildNodes() const
{
	if (m_childNodes == nullptr)
		return false;

	if (std::holds_alternative<XMLNodeChildNodeList>(*m_childNodes))
	{
		auto& nodeList = std::get<XMLNodeChildNodeList>(*m_childNodes);
		if (nodeList.empty())
			return false;
	}

	return true;
}

/*****************************************************************************/
bool XmlNode::setChildNode(std::string inValue)
{
	makeChildNodes();

	(*m_childNodes) = std::move(inValue);
	return true;
}

/*****************************************************************************/
bool XmlNode::addChildNode(Unique<XmlNode> inNode)
{
	makeChildNodes();

	if (!std::holds_alternative<XMLNodeChildNodeList>(*m_childNodes))
		(*m_childNodes) = XMLNodeChildNodeList{};

	auto& nodeList = std::get<XMLNodeChildNodeList>(*m_childNodes);
	nodeList.emplace_back(std::move(inNode));
	return true;
}

/*****************************************************************************/
bool XmlNode::removeLastChild()
{
	if (!hasChildNodes())
		return false;

	if (std::holds_alternative<std::string>(*m_childNodes))
	{
		clearChildNodes();
		return true;
	}
	else
	{
		auto& nodeList = std::get<XMLNodeChildNodeList>(*m_childNodes);
		nodeList.pop_back();
		if (nodeList.empty())
			clearChildNodes();

		return true;
	}
}

/*****************************************************************************/
bool XmlNode::clearChildNodes()
{
	if (m_childNodes != nullptr)
	{
		m_childNodes.reset();
		return true;
	}

	return false;
}

/*****************************************************************************/
void XmlNode::makeAttributes()
{
	if (m_attributes == nullptr)
	{
		m_attributes = std::make_unique<XMLNodeAttributes>();
	}
}

/*****************************************************************************/
void XmlNode::makeChildNodes()
{
	if (m_childNodes == nullptr)
	{
		m_childNodes = std::make_unique<XMLNodeChildNodes>();
	}
}

}
