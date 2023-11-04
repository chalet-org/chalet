/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_ENVIRONMENT_EMSCRIPTEN_HPP
#define CHALET_COMPILE_ENVIRONMENT_EMSCRIPTEN_HPP

#include "Compile/Environment/CompileEnvironmentLLVM.hpp"

namespace chalet
{
struct EmscriptenEnvironmentScript;

struct CompileEnvironmentEmscripten final : CompileEnvironmentLLVM
{
	explicit CompileEnvironmentEmscripten(const ToolchainType inType, BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(CompileEnvironmentEmscripten);
	~CompileEnvironmentEmscripten();

protected:
	virtual StringList getVersionCommand(const std::string& inExecutable) const final;
	virtual std::string getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const final;
	virtual bool createFromVersion(const std::string& inVersion) final;
	virtual bool validateArchitectureFromInput() final;
	virtual bool getCompilerVersionAndDescription(CompilerInfo& outInfo) const final;
	virtual bool readArchitectureTripleFromCompiler() final;
	virtual std::vector<CompilerPathStructure> getValidCompilerPaths() const final;

	virtual ToolchainType getToolchainTypeFromMacros(const std::string& inMacros) const final;

private:
	std::string getToolchainTriple(const std::string& inArch) const;

	Unique<EmscriptenEnvironmentScript> m_config;

	std::string m_emsdkRoot;
	std::string m_emsdkUpstream;
	std::string m_python;
	mutable std::string m_emcc;
	mutable std::string m_emccVersion;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_EMSCRIPTEN_HPP
