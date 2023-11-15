/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Reflect.hpp"

#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
std::string Reflect::className()
{
#if defined(CHALET_MSVC)
	std::string temp(__FUNCSIG__);
	std::string toFind{ ":className<" };
	auto start = temp.find(toFind);
	if (start == std::string::npos)
		return temp;

	temp = temp.substr(start + toFind.size());

	// Remove ">(void)";
	temp = temp.substr(0, temp.size() - 7);

	String::replaceAll(temp, "class ", "");
	String::replaceAll(temp, "struct ", "");

	String::replaceAll(temp, ",", ", ");

	return temp;
#else
	std::string temp(__PRETTY_FUNCTION__);
	auto start = temp.find("T = ");
	if (start == std::string::npos)
		return temp;

	temp = temp.substr(start + 4);

	auto end = temp.find(";");
	if (end == std::string::npos)
		end = temp.find("]");

	return temp.substr(0, end);
#endif
}
}
