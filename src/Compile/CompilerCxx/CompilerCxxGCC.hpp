/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_EXECUTABLE_CXX_GCC_HPP
#define CHALET_COMPILER_EXECUTABLE_CXX_GCC_HPP

#include "Compile/CompilerCxx/ICompilerCxx.hpp"

namespace chalet
{
struct CompilerCxxGCC : public ICompilerCxx
{
	explicit CompilerCxxGCC(const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() override;

	virtual StringList getPrecompiledHeaderCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const std::string& arch) override;
	virtual StringList getCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization) override;

	static bool addArchitectureToCommand(StringList& outArgList, const std::string& inArch, const BuildState& inState);

protected:
	virtual std::string getPragmaId() const;
	virtual StringList getWarningExclusions() const;
	virtual bool isFlagSupported(const std::string& inFlag) const;

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
	virtual void addNoExceptionsOption(StringList& outArgList) const override;
	virtual void addThreadModelCompileOption(StringList& outArgList) const override;
	virtual bool addArchitecture(StringList& outArgList, const std::string& inArch) const override;

	// Objective-C / Objective-C++
	virtual void addObjectiveCxxCompileOption(StringList& outArgList, const CxxSpecialization specialization) const;
	virtual void addObjectiveCxxRuntimeOption(StringList& outArgList, const CxxSpecialization specialization) const;

	// MacOS
	virtual bool addMacosSysRootOption(StringList& outArgList) const;

private:
	static const StringList& getSupportedX86Architectures();
	// static const StringList& getSupportedX64Architectures();
};
}

#endif // CHALET_COMPILER_EXECUTABLE_CXX_GCC_HPP