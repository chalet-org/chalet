/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Xml/XmlElement.hpp"

// https://en.wikipedia.org/wiki/XML

namespace chalet
{
/*****************************************************************************/
XmlElement::XmlElement(std::string_view inName) :
	m_name(getValidKey(inName))
{
}

/*****************************************************************************/
std::string XmlElement::dump(const uint inIndent, const int inIndentSize, const char inIndentChar) const
{
	std::string ret;

	if (m_name.empty())
		return ret;

	std::string indent(inIndent, inIndentChar);
	std::string attributes;

	if (m_attributes != nullptr)
	{
		auto& attribList = *m_attributes;
		for (const auto& [key, value] : attribList)
		{
			attributes += fmt::format(" {}=\"{}\"", key, value);
		}
	}

	ret += indent;
	if (m_commented)
		ret += "<!--";

	if (hasChild())
	{
		std::string childNodes;
		if (std::holds_alternative<std::string>(*m_child))
		{
			childNodes = std::get<std::string>(*m_child);
		}
		else
		{
			uint nextIndent = 0;
			if (inIndentSize >= 0)
			{
				childNodes += '\n';
				nextIndent = inIndent + static_cast<uint>(inIndentSize);
			}

			auto& nodeList = std::get<XmlElementList>(*m_child);
			for (auto& node : nodeList)
			{
				childNodes += node->dump(nextIndent, inIndentSize, inIndentChar);
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

	if (m_commented)
		ret += "-->";

	if (inIndentSize >= 0)
		ret += '\n';

	return ret;
}

/*****************************************************************************/
const std::string& XmlElement::name() const noexcept
{
	return m_name;
}

void XmlElement::setName(std::string_view inName)
{
	m_name = getValidKey(inName);
}

/*****************************************************************************/
bool XmlElement::hasAttributes() const
{
	return m_attributes != nullptr;
}

/*****************************************************************************/
bool XmlElement::addAttribute(std::string_view inKey, std::string_view inValue)
{
	allocateAttributes();

	m_attributes->emplace_back(std::pair<std::string, std::string>{ getValidKey(inKey), getValidAttributeValue(inValue) });
	return true;
}

/*****************************************************************************/
bool XmlElement::clearAttributes()
{
	if (m_attributes != nullptr)
	{
		m_attributes.reset();
		return true;
	}

	return false;
}

/*****************************************************************************/
bool XmlElement::hasChild() const
{
	if (m_child == nullptr)
		return false;

	if (std::holds_alternative<XmlElementList>(*m_child))
	{
		auto& nodeList = std::get<XmlElementList>(*m_child);
		if (nodeList.empty())
			return false;
	}

	return true;
}

/*****************************************************************************/
bool XmlElement::setText(std::string_view inValue)
{
	allocateChild();

	(*m_child) = getValidValue(inValue);
	return true;
}

/*****************************************************************************/
bool XmlElement::addElementWithText(std::string_view inName, std::string_view inValue)
{
	allocateChild();

	if (!std::holds_alternative<XmlElementList>(*m_child))
		(*m_child) = XmlElementList{};

	auto& nodeList = std::get<XmlElementList>(*m_child);
	auto node = std::make_unique<XmlElement>(std::move(inName));
	node->setText(std::move(inValue));

	nodeList.emplace_back(std::move(node));
	return true;
}

/*****************************************************************************/
bool XmlElement::addElementWithTextIfNotEmpty(std::string_view inName, std::string inValue)
{
	if (!inValue.empty())
		return false;

	return addElementWithText(inName, inValue);
}

/*****************************************************************************/
bool XmlElement::addElement(std::string_view inName, std::function<void(XmlElement&)> onMakeNode)
{
	allocateChild();

	if (!std::holds_alternative<XmlElementList>(*m_child))
		(*m_child) = XmlElementList{};

	auto& nodeList = std::get<XmlElementList>(*m_child);
	auto node = std::make_unique<XmlElement>(std::move(inName));
	if (onMakeNode != nullptr)
		onMakeNode(*node);

	nodeList.emplace_back(std::move(node));
	return true;
}

/*****************************************************************************/
bool XmlElement::clearChildElements()
{
	if (m_child != nullptr)
	{
		m_child.reset();
		return true;
	}

	return false;
}

/*****************************************************************************/
bool XmlElement::commented() const noexcept
{
	return m_commented;
}

void XmlElement::setCommented(const bool inValue) noexcept
{
	m_commented = inValue;
}

/*****************************************************************************/
std::string XmlElement::getValidKey(const std::string_view& inKey) const
{
	std::string ret;

	for (std::size_t i = 0; i < inKey.size(); ++i)
	{
		char c = inKey[i];
		if (c < 32 && !(c == 0x09 || c == 0x0A || c == 0x0F))
			continue;

		switch (c)
		{
			case '<':
			case '>':
			case '&':
			case '\'':
			case '"':
				continue;
			default:
				break;
		}

		ret += c;
	}

	return ret;
}

/*****************************************************************************/
std::string XmlElement::getValidAttributeValue(const std::string_view& inValue) const
{
	std::string ret;

	for (std::size_t i = 0; i < inValue.size(); ++i)
	{
		char c = inValue[i];
		if (c < 32 && !(c == 0x09 || c == 0x0A || c == 0x0F))
			continue;

		switch (c)
		{
			case '<':
				ret += "&lt;";
				continue;
			case '>':
				ret += "&gt;";
				continue;
			case '&':
				ret += "&amp;";
				continue;
			// case '\'':
			// 	ret += "&apos;";
			// 	continue;
			case '"':
				ret += "&quot;";
				continue;
			default:
				break;
		}

		ret += c;
	}

	return ret;
}

/*****************************************************************************/
std::string XmlElement::getValidValue(const std::string_view& inValue) const
{
	std::string ret;

	for (std::size_t i = 0; i < inValue.size(); ++i)
	{
		char c = inValue[i];
		if (c < 32 && !(c == 0x09 || c == 0x0A || c == 0x0F))
			continue;

		switch (c)
		{
			case '<':
				ret += "&lt;";
				continue;
			case '>':
				ret += "&gt;";
				continue;
			case '&':
				ret += "&amp;";
				continue;
			default:
				break;
		}

		ret += c;
	}

	return ret;
}

/*****************************************************************************/
void XmlElement::allocateAttributes()
{
	if (m_attributes == nullptr)
	{
		m_attributes = std::make_unique<XmlTagAttributeList>();
	}
}

/*****************************************************************************/
void XmlElement::allocateChild()
{
	if (m_child == nullptr)
	{
		m_child = std::make_unique<XmlElementChild>();
	}
}

}
