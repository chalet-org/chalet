/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_OLD_PLIST_GENERATOR_HPP
#define CHALET_OLD_PLIST_GENERATOR_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
struct OldPListGenerator
{
	OldPListGenerator();

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

#endif // CHALET_OLD_PLIST_GENERATOR_HPP
