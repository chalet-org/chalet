/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_TOOLCHAIN_PREFERENCE_HPP
#define CHALET_TOOLCHAIN_PREFERENCE_HPP

#include "Compile/Toolchain/ToolchainType.hpp"

namespace chalet
{
struct ToolchainPreference
{
	ToolchainType type = ToolchainType::Unknown;
	std::string cpp;
	std::string cc;
	std::string rc;
	std::string linker;
	std::string archiver;
};
}

#endif // CHALET_TOOLCHAIN_PREFERENCE_HPP
