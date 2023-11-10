/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
namespace Hash
{
std::string string(const std::string& inValue);
std::size_t uint64(const std::string& inValue);

template <typename... Args>
std::string getHashableString(Args&&... args);
}
}

#include "Utility/Hash.inl"
