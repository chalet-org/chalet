/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_GNU_HPP
#define CHALET_COMPILE_TOOLCHAIN_GNU_HPP

#include "Compile/Toolchain/ICompileToolchain.hpp"

#include "Compile/CompilerConfig.hpp"
#include "State/BuildState.hpp"
#include "State/Target/ProjectTarget.hpp"

namespace chalet
{
struct CompileToolchainGNU : ICompileToolchain
{
	explicit CompileToolchainGNU(const BuildState& inState, const ProjectTarget& inProject, const CompilerConfig& inConfig);

	virtual ToolchainType type() const override;

	virtual bool initialize() override;

	virtual StringList getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) override;
	virtual StringList getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) override;
	virtual StringList getCxxCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization) override;
	virtual StringList getLinkerTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) override;

protected:
	virtual StringList getLinkExclusions() const;

	// Compile
	virtual void addIncludes(StringList& outArgList) const override;
	virtual void addWarnings(StringList& outArgList) const override;
	virtual void addDefines(StringList& outArgList) const override;
	virtual void addPchInclude(StringList& outArgList) const override;
	virtual void addOptimizationOption(StringList& outArgList) const override;
	virtual void addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const override;
	virtual void addDebuggingInformationOption(StringList& outArgList) const override;
	virtual void addProfileInformationCompileOption(StringList& outArgList) const override;
	virtual void addCompileOptions(StringList& outArgList) const override;
	virtual void addDiagnosticColorOption(StringList& outArgList) const override;
	virtual void addLibStdCppCompileOption(StringList& outArgList, const CxxSpecialization specialization) const override;
	virtual void addPositionIndependentCodeOption(StringList& outArgList) const override;
	virtual void addNoRunTimeTypeInformationOption(StringList& outArgList) const override;
	virtual void addThreadModelCompileOption(StringList& outArgList) const override;
	virtual bool addArchitecture(StringList& outArgList) const override;

	// Linking
	virtual void addLibDirs(StringList& outArgList) const override;
	virtual void addLinks(StringList& outArgList) const override;
	virtual void addRunPath(StringList& outArgList) const override;
	virtual void addStripSymbolsOption(StringList& outArgList) const override;
	virtual void addLinkerOptions(StringList& outArgList) const override;
	virtual void addProfileInformationLinkerOption(StringList& outArgList) const override;
	virtual void addLinkTimeOptimizationOption(StringList& outArgList) const override;
	virtual void addThreadModelLinkerOption(StringList& outArgList) const override;
	virtual void addLinkerScripts(StringList& outArgList) const override;
	virtual void addLibStdCppLinkerOption(StringList& outArgList) const override;
	virtual void addStaticCompilerLibraryOptions(StringList& outArgList) const override;
	virtual void addPlatformGuiApplicationFlag(StringList& outArgList) const override;

	// Linking (Misc)
	virtual void startStaticLinkGroup(StringList& outArgList) const;
	virtual void endStaticLinkGroup(StringList& outArgList) const;
	virtual void startExplicitDynamicLinkGroup(StringList& outArgList) const;

	// Objective-C / Objective-C++
	virtual void addObjectiveCxxLink(StringList& outArgList) const;
	virtual void addObjectiveCxxCompileOption(StringList& outArgList, const CxxSpecialization specialization) const;
	virtual void addObjectiveCxxRuntimeOption(StringList& outArgList, const CxxSpecialization specialization) const;

	// MacOS
	virtual void addMacosSysRootOption(StringList& outArgList) const;
	virtual void addMacosFrameworkOptions(StringList& outArgList) const;

	StringList getMingwDllTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase);
	StringList getDylibTargetCommand(const std::string& outputFile, const StringList& sourceObjs);
	StringList getDynamicLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs);
	StringList getStaticLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs);
	StringList getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs);

	void addSourceObjects(StringList& outArgList, const StringList& sourceObjs) const;
	void initializeArchPresets() const;

	const ProjectTarget& m_project;
	const CompilerConfig& m_config;

	const CppCompilerType m_compilerType;

	mutable StringList m_arch86;
	// mutable StringList m_arch86_64;

private:
	std::string m_arch;
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_GNU_HPP
