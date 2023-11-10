/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LIST_HPP
#define CHALET_LIST_HPP

namespace chalet
{
namespace List
{
template <typename VectorType, class T>
void forEach(std::vector<VectorType>& inList, T* inst, void (T::*func)(VectorType&&));

template <typename VectorType>
bool addIfDoesNotExist(std::vector<VectorType>& outList, VectorType&& inValue);

bool addIfDoesNotExist(std::vector<std::string>& outList, const char* inValue);
bool addIfDoesNotExist(std::vector<std::string>& outList, const std::string& inValue);

template <typename VectorType>
void removeIfExists(std::vector<VectorType>& outList, VectorType&& inValue);

void removeIfExists(std::vector<std::string>& outList, const char* inValue);

template <typename VectorType>
bool contains(const std::vector<VectorType>& inList, const VectorType& inValue);

void removeDuplicates(std::vector<std::string>& outList);

template <typename... Args>
StringList combineRemoveDuplicates(Args&&... args);

template <typename... Args>
StringList combine(Args&&... args);
}
}

#include "Utility/List.inl"

#endif // CHALET_LIST_HPP
