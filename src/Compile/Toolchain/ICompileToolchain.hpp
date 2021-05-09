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

	virtual StringList getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) = 0;
	virtual StringList getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) = 0;
	virtual StringList getCxxCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization) = 0;
	virtual StringList getLinkerTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) = 0;

	// Compile
	virtual void addIncludes(StringList& inArgList);
	virtual void addWarnings(StringList& inArgList);
	virtual void addDefines(StringList& inArgList);
	virtual void addPchInclude(StringList& inArgList);
	virtual void addOptimizationOption(StringList& inArgList);
	virtual void addLanguageStandard(StringList& inArgList, const CxxSpecialization specialization);
	virtual void addDebuggingInformationOption(StringList& inArgList);
	virtual void addProfileInformationCompileOption(StringList& inArgList);
	virtual void addCompileOptions(StringList& inArgList);
	virtual void addDiagnosticColorOption(StringList& inArgList);
	virtual void addLibStdCppCompileOption(StringList& inArgList, const CxxSpecialization specialization);
	virtual void addPositionIndependentCodeOption(StringList& inArgList);
	virtual void addNoRunTimeTypeInformationOption(StringList& inArgList);
	virtual void addThreadModelCompileOption(StringList& inArgList);

	// Linking
	virtual void addLibDirs(StringList& inArgList);
	virtual void addLinks(StringList& inArgList);
	virtual void addRunPath(StringList& inArgList);
	virtual void addStripSymbolsOption(StringList& inArgList);
	virtual void addLinkerOptions(StringList& inArgList);
	virtual void addProfileInformationLinkerOption(StringList& inArgList);
	virtual void addLinkTimeOptimizationOption(StringList& inArgList);
	virtual void addThreadModelLinkerOption(StringList& inArgList);
	virtual void addLinkerScripts(StringList& inArgList);
	virtual void addLibStdCppLinkerOption(StringList& inArgList);
	virtual void addStaticCompilerLibraryOptions(StringList& inArgList);
	virtual void addPlatformGuiApplicationFlag(StringList& inArgList);
};

using CompileToolchain = std::unique_ptr<ICompileToolchain>;
}

#endif // CHALET_ICOMPILE_TOOLCHAIN_HPP
