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
void forEach(std::vector<VectorType>& inList, T* inst, void (T::*func)(VectorType&));

template <typename Container>
void sort(Container& inList);

template <typename VectorType>
void addIfDoesNotExist(std::vector<VectorType>& outList, VectorType&& inValue);

void addIfDoesNotExist(std::vector<std::string>& outList, const char* inValue);
void addIfDoesNotExist(std::vector<std::string>& outList, const std::string& inValue);

template <typename VectorType>
void removeIfExists(std::vector<VectorType>& outList, VectorType&& inValue);

void removeIfExists(std::vector<std::string>& outList, const char* inValue);

template <typename VectorType>
bool contains(const std::vector<VectorType>& inList, const VectorType& inValue);
}
}

#include "Utility/List.inl"

#endif // CHALET_LIST_HPP
