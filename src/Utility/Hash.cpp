/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Hash.hpp"

namespace chalet
{
/*****************************************************************************/
std::string Hash::string(const std::string& inValue)
{
	auto hash = Hash::uint64(inValue);

	std::stringstream hashHex;
	hashHex << std::hex << hash;

	return hashHex.str();
}

/*****************************************************************************/
size_t Hash::uint64(const std::string& inValue)
{
	std::hash<std::string> hash;
	return hash(inValue);
}
}
