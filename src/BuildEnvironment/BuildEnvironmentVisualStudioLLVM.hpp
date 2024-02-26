/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "BuildEnvironment/BuildEnvironmentLLVM.hpp"

namespace chalet
{
struct VisualStudioEnvironmentScript;

struct BuildEnvironmentVisualStudioLLVM final : BuildEnvironmentLLVM
{
	explicit BuildEnvironmentVisualStudioLLVM(const ToolchainType inType, BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(BuildEnvironmentVisualStudioLLVM);
	~BuildEnvironmentVisualStudioLLVM();

	virtual std::string getObjectFile(const std::string& inSource) const final;

protected:
	virtual std::string getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const final;
	virtual ToolchainType getToolchainTypeFromMacros(const std::string& inMacros) const override;
	virtual bool createFromVersion(const std::string& inVersion) final;
	virtual bool validateArchitectureFromInput() final;
	virtual std::vector<CompilerPathStructure> getValidCompilerPaths() const final;

private:
	void cacheClLocation() const;

	Unique<VisualStudioEnvironmentScript> m_config;

	mutable std::string m_cl;
};
}
