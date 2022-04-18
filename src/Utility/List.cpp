/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
bool List::addIfDoesNotExist(std::vector<std::string>& outList, const char* inValue)
{
	return List::addIfDoesNotExist(outList, std::string(inValue));
}

/*****************************************************************************/
bool List::addIfDoesNotExist(std::vector<std::string>& outList, const std::string& inValue)
{
	if (!contains(outList, inValue))
	{
		outList.push_back(inValue);
		return true;
	}

	return false;
}

/*****************************************************************************/
void List::removeIfExists(std::vector<std::string>& outList, const char* inValue)
{
	List::removeIfExists(outList, std::string(inValue));
}

/*****************************************************************************/
void List::removeDuplicates(std::vector<std::string>& outList)
{
	auto end = outList.end();
	for (auto it = outList.begin(); it != end; ++it)
	{
		end = std::remove(it + 1, end, *it);
	}

	outList.erase(end, outList.end());
}
}
