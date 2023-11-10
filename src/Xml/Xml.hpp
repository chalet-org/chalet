/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Xml/XmlElement.hpp"

namespace chalet
{
struct Xml
{
	Xml() = default;
	explicit Xml(std::string inRootName);

	std::string dump(const i32 inIndent = -1, const char inIndentChar = ' ') const;

	void addRawHeader(std::string inHeader);

	const std::string& version() const noexcept;
	void setVersion(const std::string& inVersion);

	const std::string& encoding() const noexcept;
	void setEncoding(const std::string& inVersion);

	bool standalone() const noexcept;
	void setStandalone(const bool inValue) noexcept;

	bool useHeader() const noexcept;
	void setUseHeader(const bool inValue) noexcept;

	XmlElement& root();

private:
	std::string m_version{ "1.0" };
	std::string m_encoding{ "utf-8" };

	StringList m_headers;

	XmlElement m_root;

	bool m_useHeader = true;
	bool m_standalone = false;
};
}
