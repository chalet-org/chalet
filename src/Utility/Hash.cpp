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
std::size_t Hash::uint64(const std::string& inValue)
{
	return std::hash<std::string>{}(inValue);
}
}
