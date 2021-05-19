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
struct ICompileToolchain
{
	virtual ~ICompileToolchain() = default;

	virtual ToolchainType type() const = 0;

	virtual bool preBuild();

	virtual StringList getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) = 0;
	virtual StringList getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) = 0;
	virtual StringList getCxxCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization) = 0;
	virtual StringList getLinkerTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) = 0;

	// Compile
	virtual void addIncludes(StringList& inArgList) const;
	virtual void addWarnings(StringList& inArgList) const;
	virtual void addDefines(StringList& inArgList) const;
	virtual void addPchInclude(StringList& inArgList) const;
	virtual void addOptimizationOption(StringList& inArgList) const;
	virtual void addLanguageStandard(StringList& inArgList, const CxxSpecialization specialization) const;
	virtual void addDebuggingInformationOption(StringList& inArgList) const;
	virtual void addProfileInformationCompileOption(StringList& inArgList) const;
	virtual void addCompileOptions(StringList& inArgList) const;
	virtual void addDiagnosticColorOption(StringList& inArgList) const;
	virtual void addLibStdCppCompileOption(StringList& inArgList, const CxxSpecialization specialization) const;
	virtual void addPositionIndependentCodeOption(StringList& inArgList) const;
	virtual void addNoRunTimeTypeInformationOption(StringList& inArgList) const;
	virtual void addThreadModelCompileOption(StringList& inArgList) const;

	// Linking
	virtual void addLibDirs(StringList& inArgList) const;
	virtual void addLinks(StringList& inArgList) const;
	virtual void addRunPath(StringList& inArgList) const;
	virtual void addStripSymbolsOption(StringList& inArgList) const;
	virtual void addLinkerOptions(StringList& inArgList) const;
	virtual void addProfileInformationLinkerOption(StringList& inArgList) const;
	virtual void addLinkTimeOptimizationOption(StringList& inArgList) const;
	virtual void addThreadModelLinkerOption(StringList& inArgList) const;
	virtual void addLinkerScripts(StringList& inArgList) const;
	virtual void addLibStdCppLinkerOption(StringList& inArgList) const;
	virtual void addStaticCompilerLibraryOptions(StringList& inArgList) const;
	virtual void addPlatformGuiApplicationFlag(StringList& inArgList) const;
};

using CompileToolchain = std::unique_ptr<ICompileToolchain>;
}

#endif // CHALET_ICOMPILE_TOOLCHAIN_HPP
