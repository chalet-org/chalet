/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Xcode/OldPListGenerator.hpp"

namespace chalet
{
/*****************************************************************************/
inline Json& OldPListGenerator::operator[](const char* inKey)
{
	return m_json[inKey];
}

/*****************************************************************************/
inline Json& OldPListGenerator::at(const std::string& inKey)
{
	return m_json.at(inKey);
}

}
