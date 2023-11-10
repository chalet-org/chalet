/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include <variant>

namespace chalet
{
class XmlElement;

class XmlElement
{
	using XmlTagAttributeList = std::vector<std::pair<std::string, std::string>>;
	using XmlElementList = std::vector<Unique<XmlElement>>;
	using XmlElementChild = std::variant<std::string, XmlElementList>;

public:
	XmlElement() = default;
	explicit XmlElement(std::string_view inName);

	std::string dump(const u32 inIndent, const i32 inIndentSize, const char inIndentChar) const;

	const std::string& name() const noexcept;
	void setName(std::string_view inName);

	XmlElement& getNode(std::string_view inKey);

	bool hasAttributes() const;
	bool addAttribute(std::string_view inKey, std::string_view inValue);
	bool clearAttributes();

	bool hasChild() const;
	bool setText(std::string_view inValue);
	bool addElementWithText(std::string_view inName, std::string_view inValue);
	bool addElementWithTextIfNotEmpty(std::string_view inName, std::string inValue);
	bool addElement(std::string_view inName, std::function<void(XmlElement&)> onMakeNode = nullptr);
	bool clearChildElements();

	bool commented() const noexcept;
	void setCommented(const bool inValue) noexcept;

private:
	std::string getValidKey(const std::string_view& inKey) const;
	std::string getValidAttributeValue(const std::string_view& inValue) const;
	std::string getValidValue(const std::string_view& inValue) const;

	void allocateAttributes();
	void allocateChild();

	std::string m_name;
	Unique<XmlTagAttributeList> m_attributes;
	Unique<XmlElementChild> m_child;

	bool m_commented = false;
};
}
