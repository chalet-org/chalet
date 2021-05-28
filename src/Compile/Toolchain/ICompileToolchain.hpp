/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ICOMPILE_TOOLCHAIN_HPP
#define CHALET_ICOMPILE_TOOLCHAIN_HPP

#include "Compile/Toolchain/CxxSpecialization.hpp"
#include "Compile/Toolchain/ToolchainType.hpp"

namespace chalet
{
class BuildState;

struct ICompileToolchain
{
	explicit ICompileToolchain(const BuildState& inState);
	virtual ~ICompileToolchain() = default;

	virtual ToolchainType type() const = 0;

	virtual bool initialize();

	virtual StringList getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) = 0;
	virtual StringList getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) = 0;
	virtual StringList getCxxCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization) = 0;
	virtual StringList getLinkerTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) = 0;

protected:
	virtual void addExectuable(StringList& outArgList, const std::string& inExecutable) const final;

	// Compile
	virtual void addIncludes(StringList& outArgList) const;
	virtual void addWarnings(StringList& outArgList) const;
	virtual void addDefines(StringList& outArgList) const;
	virtual void addPchInclude(StringList& outArgList) const;
	virtual void addOptimizationOption(StringList& outArgList) const;
	virtual void addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const;
	virtual void addDebuggingInformationOption(StringList& outArgList) const;
	virtual void addProfileInformationCompileOption(StringList& outArgList) const;
	virtual void addCompileOptions(StringList& outArgList) const;
	virtual void addDiagnosticColorOption(StringList& outArgList) const;
	virtual void addLibStdCppCompileOption(StringList& outArgList, const CxxSpecialization specialization) const;
	virtual void addPositionIndependentCodeOption(StringList& outArgList) const;
	virtual void addNoRunTimeTypeInformationOption(StringList& outArgList) const;
	virtual void addThreadModelCompileOption(StringList& outArgList) const;
	virtual bool addArchitecture(StringList& outArgList) const;

	// Linking
	virtual void addLibDirs(StringList& outArgList) const;
	virtual void addLinks(StringList& outArgList) const;
	virtual void addRunPath(StringList& outArgList) const;
	virtual void addStripSymbolsOption(StringList& outArgList) const;
	virtual void addLinkerOptions(StringList& outArgList) const;
	virtual void addProfileInformationLinkerOption(StringList& outArgList) const;
	virtual void addLinkTimeOptimizationOption(StringList& outArgList) const;
	virtual void addThreadModelLinkerOption(StringList& outArgList) const;
	virtual void addLinkerScripts(StringList& outArgList) const;
	virtual void addLibStdCppLinkerOption(StringList& outArgList) const;
	virtual void addStaticCompilerLibraryOptions(StringList& outArgList) const;
	virtual void addPlatformGuiApplicationFlag(StringList& outArgList) const;

	const BuildState& m_state;

	bool m_quotePaths = true;

	bool m_isMakefile = false;
	bool m_isNinja = false;
	bool m_isNative = false;
};

using CompileToolchain = std::unique_ptr<ICompileToolchain>;
}

#endif // CHALET_ICOMPILE_TOOLCHAIN_HPP
