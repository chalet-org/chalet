/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_TOOLCHAIN_PREFERENCE_HPP
#define CHALET_TOOLCHAIN_PREFERENCE_HPP

#include "Compile/Strategy/StrategyType.hpp"
#include "Compile/Toolchain/ToolchainType.hpp"

namespace chalet
{
struct ToolchainPreference
{
	mutable ToolchainType type = ToolchainType::Unknown;
	StrategyType strategy = StrategyType::Makefile;
	std::string cpp;
	std::string cc;
	std::string rc;
	std::string linker;
	std::string archiver;
	std::string profiler;

	void setType(const ToolchainType inType) const;
};
}

#endif // CHALET_TOOLCHAIN_PREFERENCE_HPP
