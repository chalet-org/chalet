/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_GNU_HPP
#define CHALET_COMPILE_TOOLCHAIN_GNU_HPP

#include "Compile/Toolchain/ICompileToolchain.hpp"

#include "BuildJson/ProjectConfiguration.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
struct CompileToolchainGNU : ICompileToolchain
{
	explicit CompileToolchainGNU(const BuildState& inState, const ProjectConfiguration& inProject);

	virtual ToolchainType type() const override;

	virtual std::string getAsmGenerateCommand(const std::string& inputFile, const std::string& outputFile) override;
	virtual std::string getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency) override;
	virtual std::string getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency) override;
	virtual std::string getCppCompileCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency) override;
	virtual std::string getObjcppCompileCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency, const bool treatAsC = false) override;
	virtual std::string getLinkerTargetCommand(const std::string& outputFile, const std::string& sourceObjs, const std::string& outputFileBase) override;

protected:
	const std::string& getIncludes();
	const std::string& getLibDirs();
	const std::string& getWarnings();
	const std::string& getDefines();
	std::string getLangStandard(const bool objectiveC = false);
	const std::string& getLinks();
	std::string getCompileFlags(const bool objectiveC = false);
	const std::string& getCompileOptions();
	const std::string& getLinkerOptions();
	const std::string& getStripSymbols();
	const std::string& getRunPathFlag();
	const std::string& getOptimizations();

	const std::string& getPchInclude();

	const BuildState& m_state;
	const ProjectConfiguration& m_project;

	const CppCompilerType m_compilerType;

	std::string m_includes;
	std::string m_libDirs;
	std::string m_warnings;
	std::string m_defines;
	std::string m_links;

	std::string m_compileOptions;
	std::string m_linkerOptions;
	std::string m_stripSymbols;
	std::string m_runPathFlag;
	std::string m_optimizations;
	std::string m_pchInclude;
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_GNU_HPP
