/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_XML_NODE_HPP
#define CHALET_XML_NODE_HPP

#include <variant>

namespace chalet
{
class XmlNode
{
	using XMLNodeAttributes = std::map<std::string, std::string>;
	using XMLNodeChildNodeList = std::vector<Unique<XmlNode>>;
	using XMLNodeChildNodes = std::variant<std::string, XMLNodeChildNodeList>;

public:
	explicit XmlNode(std::string inName);

	const std::string& name() const noexcept;

	bool hasAttribute(const std::string& inKey) const;
	const std::string& getAttribute(const std::string& inKey) const;
	bool addAttribute(const std::string& inKey, std::string inValue);
	bool setAttribute(const std::string& inKey, std::string inValue);
	bool removeAttribute(const std::string& inKey);
	bool clearAttributes();

	bool hasChildNodes() const;
	bool setChildNode(std::string inValue);
	bool addChildNode(Unique<XmlNode> inNode);
	bool removeLastChild();
	bool clearChildNodes();

private:
	friend struct Xml;

	void makeAttributes();
	void makeChildNodes();

	std::string m_name;
	Unique<XMLNodeAttributes> m_attributes;
	Unique<XMLNodeChildNodes> m_childNodes;
};
}

#endif // CHALET_XML_NODE_HPP
