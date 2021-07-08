/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_TOOLCHAIN_PREFERENCE_HPP
#define CHALET_TOOLCHAIN_PREFERENCE_HPP

#include "Compile/BuildPathStyle.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "Compile/Toolchain/ToolchainType.hpp"

namespace chalet
{
struct ToolchainPreference
{
	std::string cpp;
	std::string cc;
	std::string rc;
	std::string linker;
	std::string archiver;
	std::string profiler;

	mutable ToolchainType type = ToolchainType::Unknown;
	StrategyType strategy = StrategyType::Makefile;
	BuildPathStyle buildPathStyle = BuildPathStyle::TargetTriple;

	void setType(const ToolchainType inType) const;
};
}

#endif // CHALET_TOOLCHAIN_PREFERENCE_HPP
