/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "BuildEnvironment/BuildEnvironmentLLVM.hpp"

namespace chalet
{
struct EmscriptenEnvironmentScript;

struct BuildEnvironmentEmscripten final : BuildEnvironmentLLVM
{
	explicit BuildEnvironmentEmscripten(const ToolchainType inType, BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(BuildEnvironmentEmscripten);
	~BuildEnvironmentEmscripten();

	virtual std::string getAssemblyFile(const std::string& inSource) const final;

	const std::string& clangPath() const noexcept;
	const std::string& emsdkRoot() const noexcept;
	const std::string& emsdkUpstream() const noexcept;

protected:
	virtual std::string getExecutableExtension() const final;
	virtual std::string getSharedLibraryExtension() const final;
	virtual std::string getArchiveExtension() const final;

	virtual std::string getSystemIncludeOutputFromClang(const std::string& inExecutable, const std::string& inTempFile, const std::string& inSystemDirsFile) final;
	virtual StringList getVersionCommand(const std::string& inExecutable) const final;
	virtual std::string getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const final;
	virtual bool createFromVersion(const std::string& inVersion) final;
	virtual bool validateArchitectureFromInput() final;
	virtual bool getCompilerVersionAndDescription(CompilerInfo& outInfo) final;
	virtual bool readArchitectureTripleFromCompiler() final;
	virtual std::vector<CompilerPathStructure> getValidCompilerPaths() const final;

	virtual ToolchainType getToolchainTypeFromMacros(const std::string& inMacros) const final;

private:
	std::string getToolchainTriple(const std::string& inArch) const;

	Unique<EmscriptenEnvironmentScript> m_config;

	std::string m_emsdkRoot;
	std::string m_emsdkUpstream;
	mutable std::string m_clangPath;
	mutable std::string m_emcc;
	mutable std::string m_emccVersion;
};
}
