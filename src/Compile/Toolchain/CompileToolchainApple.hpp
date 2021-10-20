/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_APPLE_HPP
#define CHALET_COMPILE_TOOLCHAIN_APPLE_HPP

#include "Compile/Toolchain/CompileToolchainLLVM.hpp"

namespace chalet
{
struct CompileToolchainApple final : CompileToolchainLLVM
{
	explicit CompileToolchainApple(const BuildState& inState, const SourceTarget& inProject, const CompilerConfig& inConfig);

	virtual ToolchainType type() const noexcept final;

	virtual bool initialize() final;

protected:
	// Compiling
	virtual void addPchInclude(StringList& outArgList) const final;
	virtual bool addArchitecture(StringList& outArgList) const final;
	virtual bool addArchitectureOptions(StringList& outArgList) const final;
	virtual void addLibStdCppCompileOption(StringList& outArgList, const CxxSpecialization specialization) const final;
	virtual void addDiagnosticColorOption(StringList& outArgList) const final;

	// Linking
	virtual void addStripSymbolsOption(StringList& outArgList) const final;
	virtual void addThreadModelLinkerOption(StringList& outArgList) const final;
	virtual void addProfileInformationLinkerOption(StringList& outArgList) const final;
	virtual void addLibStdCppLinkerOption(StringList& outArgList) const final;

	// Objective-C / Objective-C++
	virtual void addObjectiveCxxLink(StringList& outArgList) const final;
	virtual void addObjectiveCxxCompileOption(StringList& outArgList, const CxxSpecialization specialization) const final;
	virtual void addObjectiveCxxRuntimeOption(StringList& outArgList, const CxxSpecialization specialization) const final;

	// MacOS
	virtual void addMacosMultiArchOption(StringList& outArgList, const std::string& arch) const final;
	virtual void addMacosSysRootOption(StringList& outArgList) const final;

	virtual StringList getDynamicLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) const final;

private:
	std::string m_osTarget;
	std::string m_osTargetVersion;
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_APPLE_HPP
