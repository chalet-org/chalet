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
	using XMLNodeAttributesList = std::vector<std::pair<std::string, std::string>>;
	using XMLNodeChildNodeList = std::vector<Unique<XmlNode>>;
	using XMLNodeChildNodes = std::variant<std::string, XMLNodeChildNodeList>;

public:
	explicit XmlNode(std::string inName);

	std::string toString(uint inIndent = 0) const;

	const std::string& name() const noexcept;

	bool hasAttributes() const;
	bool addAttribute(const std::string& inKey, std::string inValue);
	bool clearAttributes();

	bool hasChildNodes() const;
	bool setChildNode(std::string inValue);
	bool addChildNode(std::string inName, std::function<void(XmlNode&)> onMakeNode);
	bool clearChildNodes();

private:
	void makeAttributes();
	void makeChildNodes();

	std::string m_name;
	Unique<XMLNodeAttributesList> m_attributes;
	Unique<XMLNodeChildNodes> m_childNodes;
};
}

#endif // CHALET_XML_NODE_HPP
