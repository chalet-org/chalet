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
template <typename Container, class Inst, class MemFunc>
inline void forEach(Container& inList, Inst&& inInst, MemFunc&& inFunc);

template <typename Container>
inline void sort(Container& inList);

template <typename VectorType>
inline void addIfDoesNotExist(std::vector<VectorType>& outList, VectorType&& inValue);

template <typename VectorType>
inline bool contains(const std::vector<VectorType>& inList, const VectorType& inValue);
}
}

#include "Utility/List.inl"

#endif // CHALET_LIST_HPP
