/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_ENVIRONMENT_VISUAL_STUDIO_HPP
#define CHALET_COMPILE_ENVIRONMENT_VISUAL_STUDIO_HPP

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/VisualStudioVersion.hpp"

namespace chalet
{
class BuildState;
struct VisualStudioEnvironmentScript;

struct CompileEnvironmentVisualStudio final : ICompileEnvironment
{
	explicit CompileEnvironmentVisualStudio(const ToolchainType inType, BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(CompileEnvironmentVisualStudio);
	~CompileEnvironmentVisualStudio();

	virtual std::string getObjectFile(const std::string& inSource) const final;
	virtual std::string getDependencyFile(const std::string& inSource) const final;
	virtual std::string getAssemblyFile(const std::string& inSource) const final;
	virtual std::string getModuleDirectivesDependencyFile(const std::string& inSource) const final;
	virtual std::string getModuleBinaryInterfaceFile(const std::string& inSource) const final;
	virtual std::string getModuleBinaryInterfaceDependencyFile(const std::string& inSource) const final;

protected:
	virtual StringList getVersionCommand(const std::string& inExecutable) const final;
	virtual std::string getFullCxxCompilerString(const std::string& inVersion) const final;
	virtual bool verifyToolchain() final;

	virtual bool compilerVersionIsToolchainVersion() const final;
	virtual bool createFromVersion(const std::string& inVersion) final;
	virtual bool validateArchitectureFromInput() final;
	virtual bool getCompilerVersionAndDescription(CompilerInfo& outInfo) const final;
	virtual std::vector<CompilerPathStructure> getValidCompilerPaths() const final;

private:
	std::string makeToolchainName(const std::string& inArch) const;

	Unique<VisualStudioEnvironmentScript> m_config;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_VISUAL_STUDIO_HPP
