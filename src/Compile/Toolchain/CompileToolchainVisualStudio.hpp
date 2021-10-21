/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_VISUAL_STUDIO_HPP
#define CHALET_COMPILE_TOOLCHAIN_VISUAL_STUDIO_HPP

#include "Compile/Toolchain/ICompileToolchain.hpp"

namespace chalet
{
class BuildState;
struct SourceTarget;
struct CompilerConfig;

struct CompileToolchainVisualStudio : ICompileToolchain
{
	explicit CompileToolchainVisualStudio(const BuildState& inState, const SourceTarget& inProject, const CompilerConfig& inConfig);

	virtual ToolchainType type() const noexcept override;

	virtual bool initialize() override;

	virtual StringList getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const std::string& arch) override;
	virtual StringList getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) override;
	virtual StringList getCxxCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization) override;
	virtual StringList getLinkerTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) override;

protected:
	StringList getLinkExclusions() const;

	// Compile
	virtual void addIncludes(StringList& outArgList) const override;
	virtual void addWarnings(StringList& outArgList) const override;
	virtual void addDefines(StringList& outArgList) const override;
	virtual void addResourceDefines(StringList& outArgList) const;
	virtual void addPchInclude(StringList& outArgList) const override;
	virtual void addOptimizationOption(StringList& outArgList) const override;
	virtual void addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const override;
	virtual void addDebuggingInformationOption(StringList& outArgList) const override;
	virtual void addDiagnosticsOption(StringList& outArgList) const;
	//
	virtual void addCompileOptions(StringList& outArgList) const override;
	virtual void addNoRunTimeTypeInformationOption(StringList& outArgList) const override;
	virtual void addNoExceptionsOption(StringList& outArgList) const override;
	virtual void addThreadModelCompileOption(StringList& outArgList) const override;
	virtual void addWholeProgramOptimization(StringList& outArgList) const;

	// Linking
	virtual void addLibDirs(StringList& outArgList) const override;
	virtual void addLinks(StringList& outArgList) const override;
	virtual void addLinkerOptions(StringList& outArgList) const override;
	virtual void addCgThreads(StringList& outArgList) const;
	virtual void addSubSystem(StringList& outArgList) const override;
	virtual void addEntryPoint(StringList& outArgList) const override;

	// General
	virtual void addTargetPlatformArch(StringList& outArgList) const;

private:
	StringList getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase);
	StringList getStaticLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase);
	StringList getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase);

	void addSourceObjects(StringList& outArgList, const StringList& sourceObjs) const;
	void addPrecompiledHeaderLink(StringList outArgList) const;

	const CppCompilerType m_compilerType;

	std::string m_pchSource;
	std::string m_pchMinusLocation;
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_VISUAL_STUDIO_HPP
