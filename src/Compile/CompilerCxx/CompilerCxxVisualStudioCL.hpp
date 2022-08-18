/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_EXECUTABLE_CXX_VISUAL_STUDIO_CL_HPP
#define CHALET_COMPILER_EXECUTABLE_CXX_VISUAL_STUDIO_CL_HPP

#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"
#include "Compile/CompilerCxx/ICompilerCxx.hpp"

namespace chalet
{
struct CompilerCxxVisualStudioCL : public ICompilerCxx
{
	explicit CompilerCxxVisualStudioCL(const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() override;

	virtual StringList getPrecompiledHeaderCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const std::string& arch) override;
	virtual StringList getCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization) override;
	virtual void getCommandOptions(StringList& outArgList, const CxxSpecialization specialization) override;

	virtual StringList getModuleCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependencyFile, const std::string& interfaceFile, const StringList& inModuleReferences, const StringList& inHeaderUnits, const ModuleFileType inType) override;

	virtual void addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const override;

protected:
	bool createPrecompiledHeaderSource();
	bool configureWarnings();

	virtual void addIncludes(StringList& outArgList) const override;
	virtual void addWarnings(StringList& outArgList) const override;
	virtual void addDefines(StringList& outArgList) const override;
	virtual void addPchInclude(StringList& outArgList) const override;
	virtual void addOptimizations(StringList& outArgList) const override;
	virtual void addCompileOptions(StringList& outArgList) const override;
	virtual void addCharsets(StringList& outArgList) const override;
	virtual void addNoRunTimeTypeInformationOption(StringList& outArgList) const override;
	virtual void addNoExceptionsOption(StringList& outArgList) const override;
	virtual void addFastMathOption(StringList& outArgList) const override;
	virtual void addThreadModelCompileOption(StringList& outArgList) const override;
	virtual void addSanitizerOptions(StringList& outArgList) const override;

	//
	virtual void addDiagnostics(StringList& outArgList) const;
	virtual void addWholeProgramOptimization(StringList& outArgList) const;

	virtual void addNativeJustMyCodeDebugging(StringList& outArgList) const;
	virtual void addBufferSecurityCheck(StringList& outArgList) const;
	virtual void addStandardBehaviors(StringList& outArgList) const;
	virtual void addAdditionalSecurityChecks(StringList& outArgList) const;
	virtual void addCallingConvention(StringList& outArgList) const;
	virtual void addFullPathSourceCode(StringList& outArgList) const;
	virtual void addStandardsConformance(StringList& outArgList) const;
	virtual void addSeparateProgramDatabase(StringList& outArgList) const;
	virtual void addAdditionalOptions(StringList& outArgList) const;
	virtual void addProgramDatabaseOutput(StringList& outArgList) const;
	virtual void addExternalWarnings(StringList& outArgList) const;
	virtual void addRuntimeErrorChecks(StringList& outArgList) const;
	virtual void addFunctionLevelLinking(StringList& outArgList) const;
	virtual void addGenerateIntrinsicFunctions(StringList& outArgList) const;

	virtual void addCppCoroutines(StringList& outArgList) const;

private:
	std::string getWarningLevel(const MSVCWarningLevel inLevel) const;

	CommandAdapterMSVC m_msvcAdapter;

	std::string m_ifcDirectory;
	std::string m_warningFlag;
};
}

#endif // CHALET_COMPILER_EXECUTABLE_CXX_VISUAL_STUDIO_CL_HPP
