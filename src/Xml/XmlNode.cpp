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
std::string XmlNode::toString(uint inIndent) const
{
	std::string ret;
	std::string indent(inIndent, '\t');
	std::string attributes;

	if (m_attributes != nullptr)
	{
	}

	ret += indent;
	if (hasChildNodes())
	{
		std::string childNodes;
		if (std::holds_alternative<std::string>(*m_childNodes))
		{
			childNodes = std::get<std::string>(*m_childNodes);
		}
		else
		{
			auto& nodeList = std::get<XMLNodeChildNodeList>(*m_childNodes);
			childNodes += '\n';
			uint nextIndent = ++inIndent;
			for (auto& node : nodeList)
			{
				childNodes += node->toString(nextIndent);
			}
			childNodes += indent;
		}
		ret += fmt::format("<{name}{attributes}>{childNodes}</{name}>",
			fmt::arg("name", m_name),
			FMT_ARG(attributes),
			FMT_ARG(childNodes));
	}
	else
	{
		ret += fmt::format("<{}{} />", m_name, attributes);
	}

	ret += '\n';

	return ret;
}

/*****************************************************************************/
const std::string& XmlNode::name() const noexcept
{
	return m_name;
}

/*****************************************************************************/
bool XmlNode::hasAttributes() const
{
	return m_attributes != nullptr
}

/*****************************************************************************/
bool XmlNode::addAttribute(const std::string& inKey, std::string inValue)
{
	makeAttributes();

	m_attributes->emplace_back(std::make_pair<std::string, std::string>(inKey, std::move(inValue)));
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
bool XmlNode::addChildNode(std::string inName, std::function<void(XmlNode&)> onMakeNode)
{
	makeChildNodes();

	if (!std::holds_alternative<XMLNodeChildNodeList>(*m_childNodes))
		(*m_childNodes) = XMLNodeChildNodeList{};

	auto& nodeList = std::get<XMLNodeChildNodeList>(*m_childNodes);

	auto node = std::make_unique<XmlNode>(std::move(inName));
	if (onMakeNode != nullptr)
		onMakeNode(*node);

	nodeList.emplace_back(std::move(node));
	return true;
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
		m_attributes = std::make_unique<XMLNodeAttributesList>();
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
