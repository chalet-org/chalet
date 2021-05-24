/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_MSVC_HPP
#define CHALET_COMPILE_TOOLCHAIN_MSVC_HPP

#include "Compile/Toolchain/ICompileToolchain.hpp"

#include "BuildJson/ProjectConfiguration.hpp"
#include "Compile/CompilerConfig.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
struct CompileToolchainMSVC final : ICompileToolchain
{
	explicit CompileToolchainMSVC(const BuildState& inState, const ProjectConfiguration& inProject, const CompilerConfig& inConfig);

	virtual ToolchainType type() const final;

	virtual bool preBuild() final;

	virtual StringList getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) final;
	virtual StringList getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) final;
	virtual StringList getCxxCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization) final;
	virtual StringList getLinkerTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) final;

private:
	StringList getLinkExclusions() const;

	// Compile
	virtual void addIncludes(StringList& outArgList) const final;
	virtual void addWarnings(StringList& outArgList) const final;
	virtual void addDefines(StringList& outArgList) const final;
	void addResourceDefines(StringList& outArgList) const;
	void addExceptionHandlingModel(StringList& outArgList) const;
	virtual void addPchInclude(StringList& outArgList) const final;
	virtual void addOptimizationOption(StringList& outArgList) const final;
	virtual void addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const final;
	virtual void addDebuggingInformationOption(StringList& outArgList) const final;
	//
	virtual void addCompileOptions(StringList& outArgList) const final;
	virtual void addNoRunTimeTypeInformationOption(StringList& outArgList) const final;
	virtual void addThreadModelCompileOption(StringList& outArgList) const final;
	void addWholeProgramOptimization(StringList& outArgList) const;

	// Linking
	virtual void addLibDirs(StringList& outArgList) const final;
	virtual void addLinks(StringList& outArgList) const final;
	void addCgThreads(StringList& outArgList) const;
	void addSubSystem(StringList& outArgList) const;

	// General
	void addTargetPlatformArch(StringList& outArgList) const;

	StringList getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs);
	StringList getStaticLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase);
	StringList getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase);

	std::string getPathCommand(std::string_view inCmd, const std::string& inPath) const;

	void addSourceObjects(StringList& outArgList, const StringList& sourceObjs) const;
	void addPrecompiledHeaderLink(StringList outArgList) const;

	const ProjectConfiguration& m_project;
	const CompilerConfig& m_config;

	const CppCompilerType m_compilerType;

	std::string m_pchSource;
	std::string m_pchMinusLocation;
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_MSVC_HPP
