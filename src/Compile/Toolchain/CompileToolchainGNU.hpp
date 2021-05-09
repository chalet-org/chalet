/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_GNU_HPP
#define CHALET_COMPILE_TOOLCHAIN_GNU_HPP

#include "Compile/Toolchain/ICompileToolchain.hpp"

#include "BuildJson/ProjectConfiguration.hpp"
#include "Compile/CompilerConfig.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
struct CompileToolchainGNU : ICompileToolchain
{
	explicit CompileToolchainGNU(const BuildState& inState, const ProjectConfiguration& inProject, const CompilerConfig& inConfig);

	virtual ToolchainType type() const override;

	virtual StringList getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) override;
	virtual StringList getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) override;
	virtual StringList getCxxCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization) override;
	virtual StringList getLinkerTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) override;

protected:
	// Compile
	virtual void addIncludes(StringList& inArgList) override;
	virtual void addWarnings(StringList& inArgList) override;
	virtual void addDefines(StringList& inArgList) override;
	virtual void addPchInclude(StringList& inArgList) override;
	virtual void addOptimizationOption(StringList& inArgList) override;
	virtual void addLanguageStandard(StringList& inArgList, const CxxSpecialization specialization) override;
	virtual void addDebuggingInformationOption(StringList& inArgList) override;
	virtual void addProfileInformationCompileOption(StringList& inArgList) override;
	virtual void addCompileOptions(StringList& inArgList) override;
	virtual void addDiagnosticColorOption(StringList& inArgList) override;
	virtual void addLibStdCppCompileOption(StringList& inArgList, const CxxSpecialization specialization) override;
	virtual void addPositionIndependentCodeOption(StringList& inArgList) override;
	virtual void addNoRunTimeTypeInformationOption(StringList& inArgList) override;
	virtual void addThreadModelCompileOption(StringList& inArgList) override;

	// Linking
	virtual void addLibDirs(StringList& inArgList) override;
	virtual void addLinks(StringList& inArgList) override;
	virtual void addRunPath(StringList& inArgList) override;
	virtual void addStripSymbolsOption(StringList& inArgList) override;
	virtual void addLinkerOptions(StringList& inArgList) override;
	virtual void addProfileInformationLinkerOption(StringList& inArgList) override;
	virtual void addLinkTimeOptimizationOption(StringList& inArgList) override;
	virtual void addThreadModelLinkerOption(StringList& inArgList) override;
	virtual void addLinkerScripts(StringList& inArgList) override;
	virtual void addLibStdCppLinkerOption(StringList& inArgList) override;
	virtual void addStaticCompilerLibraryOptions(StringList& inArgList) override;
	virtual void addPlatformGuiApplicationFlag(StringList& inArgList) override;

	// Linking (Misc)
	virtual void startStaticLinkGroup(StringList& inArgList);
	virtual void endStaticLinkGroup(StringList& inArgList);
	virtual void startExplicitDynamicLinkGroup(StringList& inArgList);

	// Objective-C / Objective-C++
	virtual void addObjectiveCxxLink(StringList& inArgList);
	virtual void addObjectiveCxxCompileOption(StringList& inArgList, const CxxSpecialization specialization);
	virtual void addObjectiveCxxRuntimeOption(StringList& inArgList, const CxxSpecialization specialization);

	// MacOS
	virtual void addMacosSysRootOption(StringList& inArgList);
	virtual void addMacosFrameworkOptions(StringList& inArgList);

	StringList getMingwDllTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase);
	StringList getDylibTargetCommand(const std::string& outputFile, const StringList& sourceObjs);
	StringList getDynamicLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs);
	StringList getStaticLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs);
	StringList getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs);

	void addSourceObjects(StringList& inArgList, const StringList& sourceObjs);

	const BuildState& m_state;
	const ProjectConfiguration& m_project;
	const CompilerConfig& m_config;

	const CppCompilerType m_compilerType;

	bool m_quotePaths = true;
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_GNU_HPP
