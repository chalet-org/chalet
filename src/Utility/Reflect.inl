/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Reflect.hpp"

namespace chalet
{
template <typename T>
std::string Reflect::className()
{
	// just gcc & clang at the moment
	std::string temp(__PRETTY_FUNCTION__);
	auto start = temp.find("T = ");
	if (start == std::string::npos)
		return temp;

	temp = temp.substr(start + 4);

	auto end = temp.find(';');
	if (end == std::string::npos)
		end = temp.find(']');

	return temp.substr(0, end);
}
}