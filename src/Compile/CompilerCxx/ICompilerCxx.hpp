/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ICOMPILER_EXECUTABLE_CXX_HPP
#define CHALET_ICOMPILER_EXECUTABLE_CXX_HPP

#include "Compile/CxxSpecialization.hpp"
#include "Compile/IToolchainExecutableBase.hpp"
#include "Compile/ModuleFileType.hpp"

namespace chalet
{
struct ICompilerCxx : public IToolchainExecutableBase
{
	explicit ICompilerCxx(const BuildState& inState, const SourceTarget& inProject);

	[[nodiscard]] static Unique<ICompilerCxx> make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() = 0;

	virtual StringList getPrecompiledHeaderCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const std::string& arch) = 0;
	virtual StringList getCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization) = 0;

	virtual StringList getModuleCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependencyFile, const std::string& interfaceFile, const StringList& inModuleReferences, const StringList& inHeaderUnits, const ModuleFileType inType);

protected:
	virtual void addIncludes(StringList& outArgList) const;
	virtual void addWarnings(StringList& outArgList) const;
	virtual void addDefines(StringList& outArgList) const;
	virtual void addPchInclude(StringList& outArgList) const;
	virtual void addOptimizations(StringList& outArgList) const;
	virtual void addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const;
	virtual void addDebuggingInformationOption(StringList& outArgList) const;
	virtual void addProfileInformationCompileOption(StringList& outArgList) const;
	virtual void addSanitizerOptions(StringList& outArgList) const;
	virtual void addCompileOptions(StringList& outArgList) const;
	virtual void addDiagnosticColorOption(StringList& outArgList) const;
	virtual void addLibStdCppCompileOption(StringList& outArgList, const CxxSpecialization specialization) const;
	virtual void addPositionIndependentCodeOption(StringList& outArgList) const;
	virtual void addNoRunTimeTypeInformationOption(StringList& outArgList) const;
	virtual void addNoExceptionsOption(StringList& outArgList) const;
	virtual void addThreadModelCompileOption(StringList& outArgList) const;
	virtual bool addArchitecture(StringList& outArgList, const std::string& inArch) const;

	uint m_versionMajorMinor = 0;
	uint m_versionPatch = 0;
};
}

#endif // CHALET_ICOMPILER_EXECUTABLE_CXX_HPP
