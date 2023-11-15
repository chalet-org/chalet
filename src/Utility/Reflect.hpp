/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
namespace Reflect
{
template <typename T>
std::string className();
}
}

#include "Utility/Reflect.inl"

// poor man's reflection
#define CHALET_REFLECT(x) std::string(#x)
