/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_MSVC_HPP
#define CHALET_COMPILE_TOOLCHAIN_MSVC_HPP

#include "Compile/Toolchain/ICompileToolchain.hpp"

#include "BuildJson/ProjectConfiguration.hpp"
#include "Compile/CompilerConfig.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
struct CompileToolchainMSVC final : ICompileToolchain
{
	explicit CompileToolchainMSVC(const BuildState& inState, const ProjectConfiguration& inProject, const CompilerConfig& inConfig);

	virtual ToolchainType type() const final;

	virtual StringList getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) final;
	virtual StringList getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) final;
	virtual StringList getCxxCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization) final;
	virtual StringList getLinkerTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) final;

private:
	virtual void addIncludes(StringList& inArgList) const final;
	// virtual void addLibDirs(StringList& inArgList) final;
	// virtual void addWarnings(StringList& inArgList) final;
	virtual void addDefines(StringList& inArgList) const final;
	virtual void addLanguageStandard(StringList& inArgList, const CxxSpecialization specialization) const final;

	const BuildState& m_state;
	const ProjectConfiguration& m_project;
	const CompilerConfig& m_config;

	const CppCompilerType m_compilerType;

	bool m_quotePaths = true;
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_MSVC_HPP
