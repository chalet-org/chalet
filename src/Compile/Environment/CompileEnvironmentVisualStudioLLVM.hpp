/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Environment/CompileEnvironmentLLVM.hpp"

namespace chalet
{
struct VisualStudioEnvironmentScript;

struct CompileEnvironmentVisualStudioLLVM final : CompileEnvironmentLLVM
{
	explicit CompileEnvironmentVisualStudioLLVM(const ToolchainType inType, BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(CompileEnvironmentVisualStudioLLVM);
	~CompileEnvironmentVisualStudioLLVM();

	virtual std::string getObjectFile(const std::string& inSource) const final;

protected:
	virtual std::string getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const final;
	virtual ToolchainType getToolchainTypeFromMacros(const std::string& inMacros) const override;
	virtual bool createFromVersion(const std::string& inVersion) final;
	virtual bool validateArchitectureFromInput() final;
	virtual std::vector<CompilerPathStructure> getValidCompilerPaths() const final;

private:
	Unique<VisualStudioEnvironmentScript> m_config;
};
}
