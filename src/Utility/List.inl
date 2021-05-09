/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename Container, class Inst, class MemFunc>
inline void List::forEach(Container& inList, Inst&& inInst, MemFunc&& inFunc)
{
	std::for_each(inList.begin(), inList.end(), std::bind(std::forward<MemFunc>(inFunc), std::forward<Inst>(inInst), std::placeholders::_1));
}

/*****************************************************************************/
template <typename Container>
inline void List::sort(Container& inList)
{
	std::sort(inList.begin(), inList.end());
}

/*****************************************************************************/
template <typename VectorType>
inline void List::addIfDoesNotExist(std::vector<VectorType>& outList, VectorType&& inValue)
{
	if (!contains(outList, inValue))
		outList.push_back(std::forward<VectorType>(inValue));
}

inline void List::addIfDoesNotExist(std::vector<std::string>& outList, const char* inValue)
{
	List::addIfDoesNotExist(outList, std::string(inValue));
}

inline void List::addIfDoesNotExist(std::vector<std::string>& outList, const std::string& inValue)
{
	if (!contains(outList, inValue))
		outList.push_back(inValue);
}

/*****************************************************************************/
template <typename VectorType>
inline void List::removeIfExists(std::vector<VectorType>& outList, VectorType&& inValue)
{
	outList.erase(std::remove(outList.begin(), outList.end(), inValue), outList.end());
}

inline void List::removeIfExists(std::vector<std::string>& outList, const char* inValue)
{
	List::removeIfExists(outList, std::string(inValue));
}

/*****************************************************************************/
template <typename VectorType>
inline bool List::contains(const std::vector<VectorType>& inList, const VectorType& inValue)
{
	return std::find(inList.begin(), inList.end(), inValue) != inList.end();
}
}
