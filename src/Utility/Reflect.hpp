/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_REFLECT_HPP
#define CHALET_REFLECT_HPP

namespace chalet
{
namespace Reflect
{
// avoid for compatiblity (just uses __PRETTY_FUNCTION__ for now)
template <typename T>
std::string className();
}
}

#include "Utility/Reflect.inl"

// poor man's reflection
#define CHALET_REFLECT(x) std::string(#x)

#endif // CHALET_REFLECT_HPP
