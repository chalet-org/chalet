/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_GNU_HPP
#define CHALET_COMPILE_TOOLCHAIN_GNU_HPP

#include "Compile/CompilerConfig.hpp"
#include "Compile/Toolchain/ICompileToolchain.hpp"

#include "BuildJson/ProjectConfiguration.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
struct CompileToolchainGNU : ICompileToolchain
{
	explicit CompileToolchainGNU(const BuildState& inState, const ProjectConfiguration& inProject, const CompilerConfig& inConfig);

	virtual ToolchainType type() const override;

	virtual std::string getAsmGenerateCommand(const std::string& inputFile, const std::string& outputFile) override;
	virtual StringList getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) override;
	virtual StringList getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) override;
	virtual StringList getCxxCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization) override;
	virtual StringList getLinkerTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) override;

protected:
	void addIncludes(StringList& inArgList);
	void addLibDirs(StringList& inArgList);
	void addWarnings(StringList& inArgList);
	void addDefines(StringList& inArgList);
	void addLinks(StringList& inArgList);
	void addPchInclude(StringList& inArgList);
	void addOptimizationFlag(StringList& inArgList);
	void addStripSymbols(StringList& inArgList);
	void addRunPath(StringList& inArgList);
	void addLanguageStandard(StringList& inArgList, const CxxSpecialization specialization);
	void addCompileFlags(StringList& inArgList, const bool forPch, const CxxSpecialization specialization);
	void addObjectiveCxxCompileFlag(StringList& inArgList, const CxxSpecialization specialization);
	void addOtherCompileOptions(StringList& inArgList, const CxxSpecialization specialization);
	void addLinkerOptions(StringList& inArgList);

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
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_GNU_HPP
