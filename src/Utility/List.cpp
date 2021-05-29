/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
void List::addIfDoesNotExist(std::vector<std::string>& outList, const char* inValue)
{
	List::addIfDoesNotExist(outList, std::string(inValue));
}

/*****************************************************************************/
void List::addIfDoesNotExist(std::vector<std::string>& outList, const std::string& inValue)
{
	if (!contains(outList, inValue))
		outList.push_back(inValue);
}

/*****************************************************************************/
void List::removeIfExists(std::vector<std::string>& outList, const char* inValue)
{
	List::removeIfExists(outList, std::string(inValue));
}
}
