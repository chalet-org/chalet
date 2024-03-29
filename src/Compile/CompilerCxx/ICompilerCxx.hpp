/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/IToolchainExecutableBase.hpp"
#include "Compile/ModuleFileType.hpp"
#include "State/SourceType.hpp"

namespace chalet
{
struct ICompilerCxx : public IToolchainExecutableBase
{
	explicit ICompilerCxx(const BuildState& inState, const SourceTarget& inProject);

	[[nodiscard]] static Unique<ICompilerCxx> make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() = 0;

	virtual StringList getPrecompiledHeaderCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency, const std::string& arch) = 0;
	virtual StringList getCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency, const SourceType derivative) = 0;
	virtual void getCommandOptions(StringList& outArgList, const SourceType derivative) = 0;

	virtual StringList getModuleCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependencyFile, const std::string& interfaceFile, const StringList& inModuleReferences, const StringList& inHeaderUnits, const ModuleFileType inType);

	virtual void addLanguageStandard(StringList& outArgList, const SourceType derivative) const;

	bool forceActualPchPath() const noexcept;
	void setForceActualPchPath(const bool inValue) noexcept;

protected:
	virtual bool addExecutable(StringList& outArgList) const;

	virtual bool precompiledHeaderAllowedForSourceType(const SourceType derivative) const final;

	virtual void addSourceFileInterpretation(StringList& outArgList, const SourceType derivative) const;
	virtual void addIncludes(StringList& outArgList) const;
	virtual void addWarnings(StringList& outArgList) const;
	virtual void addDefines(StringList& outArgList) const;
	virtual void addPchInclude(StringList& outArgList, const SourceType derivative) const;
	virtual void addOptimizations(StringList& outArgList) const;
	virtual void addDebuggingInformationOption(StringList& outArgList) const;
	virtual void addProfileInformation(StringList& outArgList) const;
	virtual void addSanitizerOptions(StringList& outArgList) const;
	virtual void addCompileOptions(StringList& outArgList) const;
	virtual void addDiagnosticColorOption(StringList& outArgList) const;
	virtual void addCharsets(StringList& outArgList) const;
	virtual void addPositionIndependentCodeOption(StringList& outArgList) const;
	virtual void addNoRunTimeTypeInformationOption(StringList& outArgList) const;
	virtual void addNoExceptionsOption(StringList& outArgList) const;
	virtual void addFastMathOption(StringList& outArgList) const;
	virtual void addThreadModelCompileOption(StringList& outArgList) const;
	virtual bool addArchitecture(StringList& outArgList, const std::string& inArch) const;

	u32 m_versionMajorMinor = 0;
	u32 m_versionPatch = 0;

	bool m_forceActualPchPath = false;
};
}
