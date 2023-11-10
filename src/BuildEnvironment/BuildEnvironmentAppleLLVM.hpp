/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "BuildEnvironment/BuildEnvironmentLLVM.hpp"

namespace chalet
{
struct BuildEnvironmentAppleLLVM final : BuildEnvironmentLLVM
{
	explicit BuildEnvironmentAppleLLVM(const ToolchainType inType, BuildState& inState);

protected:
	virtual std::string getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const final;
	virtual ToolchainType getToolchainTypeFromMacros(const std::string& inMacros) const override;
};
}
