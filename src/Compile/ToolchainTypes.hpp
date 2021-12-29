/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_TOOLCHAIN_TYPES_HPP
#define CHALET_TOOLCHAIN_TYPES_HPP

#include <Compile/ToolchainType.hpp>

namespace chalet
{
namespace ToolchainTypes
{
std::string getTypeName(const ToolchainType inType) noexcept;
StringList getNotTypes(const std::string& inType) noexcept;
}
}

#endif // CHALET_TOOLCHAIN_TYPES_HPP
