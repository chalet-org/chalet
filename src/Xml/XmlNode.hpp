/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_XML_NODE_HPP
#define CHALET_XML_NODE_HPP

#include <variant>

namespace chalet
{
class XmlNode;

class XmlNode
{
	using XmlNodeAttributesList = std::vector<std::pair<std::string, std::string>>;
	using XmlNodeChildNodeList = std::vector<Unique<XmlNode>>;
	using XmlNodeChildNodes = std::variant<std::string, XmlNodeChildNodeList>;

public:
	XmlNode() = default;
	explicit XmlNode(std::string_view inName);

	std::string dump(const uint inIndent, const int inIndentSize, const char inIndentChar) const;

	const std::string& name() const noexcept;
	void setName(std::string_view inName);

	bool hasAttributes() const;
	bool addAttribute(std::string_view inKey, std::string_view inValue);
	bool clearAttributes();

	bool hasChildNodes() const;
	bool setChildNode(std::string_view inValue);
	bool addChildNode(std::string_view inName, std::string_view inValue);
	bool addChildNode(std::string_view inName, std::function<void(XmlNode&)> onMakeNode);
	bool clearChildNodes();

	bool commented() const noexcept;
	void setCommented(const bool inValue) noexcept;

private:
	std::string getValidKey(const std::string_view& inKey) const;
	std::string getValidAttributeValue(const std::string_view& inValue) const;
	std::string getValidValue(const std::string_view& inValue) const;

	void makeAttributes();
	void makeChildNodes();

	std::string m_name;
	Unique<XmlNodeAttributesList> m_attributes;
	Unique<XmlNodeChildNodes> m_childNodes;

	bool m_commented = false;
};
}

#endif // CHALET_XML_NODE_HPP
