/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename VectorType, class T>
void List::forEach(std::vector<VectorType>& inList, T* inst, void (T::*func)(VectorType&&))
{
	for (auto&& item : inList)
	{
		std::invoke(func, inst, std::move(item));
	}
}

/*****************************************************************************/
template <typename Container>
void List::sort(Container& inList)
{
	std::sort(inList.begin(), inList.end());
}

/*****************************************************************************/
template <typename VectorType>
void List::addIfDoesNotExist(std::vector<VectorType>& outList, VectorType&& inValue)
{
	if (std::find(outList.begin(), outList.end(), inValue) == outList.end())
		outList.emplace_back(std::forward<VectorType>(inValue));
}

/*****************************************************************************/
template <typename VectorType>
void List::removeIfExists(std::vector<VectorType>& outList, VectorType&& inValue)
{
	outList.erase(std::remove(outList.begin(), outList.end(), inValue), outList.end());
}

/*****************************************************************************/
template <typename VectorType>
bool List::contains(const std::vector<VectorType>& inList, const VectorType& inValue)
{
	return std::find(inList.begin(), inList.end(), inValue) != inList.end();
}
}
