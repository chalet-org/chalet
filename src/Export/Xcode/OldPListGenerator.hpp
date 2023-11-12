/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"

namespace chalet
{
struct OldPListGenerator
{
	OldPListGenerator() = default;

	inline Json& operator[](const char* inKey);
	inline Json& at(const std::string& inKey);

	void dumpToTerminal();

	std::string getContents(const StringList& inSingleLineSections = StringList{}) const;

private:
	std::string getNodeAsPListFormat(const Json& inJson, const size_t indent = 1) const;
	std::string getNodeAsPListString(const Json& inJson) const;

	Json m_json;
};
}

#include "Export/Xcode/OldPListGenerator.inl"
