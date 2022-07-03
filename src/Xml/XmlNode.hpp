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
	explicit XmlNode(std::string inName);

	std::string dump(const uint inIndent, const int inIndentSize, const char inIndentChar) const;

	const std::string& name() const noexcept;
	void setName(std::string inName);

	bool hasAttributes() const;
	bool addAttribute(const std::string& inKey, std::string inValue);
	bool clearAttributes();

	bool hasChildNodes() const;
	bool setChildNode(std::string inValue);
	bool addChildNode(std::string inName, std::string inValue);
	bool addChildNode(std::string inName, std::function<void(XmlNode&)> onMakeNode);
	bool clearChildNodes();

private:
	void makeAttributes();
	void makeChildNodes();

	std::string m_name;
	Unique<XmlNodeAttributesList> m_attributes;
	Unique<XmlNodeChildNodes> m_childNodes;
};
}

#endif // CHALET_XML_NODE_HPP
