/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CompilerCxx/ICompilerCxx.hpp"

namespace chalet
{
struct CompilerCxxGCC : public ICompilerCxx
{
	explicit CompilerCxxGCC(const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() override;

	virtual StringList getPrecompiledHeaderCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency, const std::string& arch) override;
	virtual StringList getCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency, const SourceType derivative) override;
	virtual void getCommandOptions(StringList& outArgList, const SourceType derivative) override;

	virtual StringList getModuleCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependencyFile, const std::string& interfaceFile, const StringList& inModuleReferences, const StringList& inHeaderUnits, const ModuleFileType inType) override;

	static bool addArchitectureToCommand(StringList& outArgList, const std::string& inArch, const BuildState& inState);
	static void addSanitizerOptions(StringList& outArgList, const BuildState& inState);

	virtual void addLanguageStandard(StringList& outArgList, const SourceType derivative) const override;

protected:
	bool configureWarnings();

	virtual std::string getPragmaId() const;
	virtual StringList getWarningExclusions() const;
	virtual bool isFlagSupported(const std::string& inFlag) const;

	virtual void addSourceFileInterpretation(StringList& outArgList, const SourceType derivative) const override;
	virtual void addSourceFileInterpretation(StringList& outArgList, const ModuleFileType moduleType) const;
	virtual void addIncludes(StringList& outArgList) const override;
	virtual void addWarnings(StringList& outArgList) const override;
	virtual void addDefines(StringList& outArgList) const override;
	virtual void addPchInclude(StringList& outArgList, const SourceType derivative) const override;
	virtual void addOptimizations(StringList& outArgList) const override;
	virtual void addDebuggingInformationOption(StringList& outArgList) const override;
	virtual void addProfileInformation(StringList& outArgList) const override;
	virtual void addSanitizerOptions(StringList& outArgList) const override;
	virtual void addCompileOptions(StringList& outArgList) const override;
	virtual void addCharsets(StringList& outArgList) const override;
	virtual void addDiagnosticColorOption(StringList& outArgList) const override;
	virtual void addLibStdCppCompileOption(StringList& outArgList, const SourceType derivative) const;
	virtual void addPositionIndependentCodeOption(StringList& outArgList) const override;
	virtual void addNoRunTimeTypeInformationOption(StringList& outArgList) const override;
	virtual void addNoExceptionsOption(StringList& outArgList) const override;
	virtual void addFastMathOption(StringList& outArgList) const override;
	virtual void addThreadModelCompileOption(StringList& outArgList) const override;
	virtual bool addArchitecture(StringList& outArgList, const std::string& inArch) const override;

	virtual bool addSystemRootOption(StringList& outArgList) const;
	virtual bool addSystemIncludes(StringList& outArgList) const;
	virtual void addLinkTimeOptimizations(StringList& outArgList) const;
	virtual void addCppCoroutines(StringList& outArgList) const;
	virtual void addCppConcepts(StringList& outArgList) const;

	// Objective-C / Objective-C++
	virtual void addObjectiveCxxRuntimeOption(StringList& outArgList, const SourceType derivative) const;

private:
	static const StringList& getSupportedX86Architectures();
	// static const StringList& getSupportedX64Architectures();

	StringList m_warnings;
};
}
