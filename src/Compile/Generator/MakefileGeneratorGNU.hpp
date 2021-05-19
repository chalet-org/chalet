/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MAKEFILE_GENERATOR_GNU_HPP
#define CHALET_MAKEFILE_GENERATOR_GNU_HPP

#include "Compile/Generator/IStrategyGenerator.hpp"
#include "Compile/Toolchain/ICompileToolchain.hpp"
#include "State/BuildState.hpp"
#include "State/SourceOutputs.hpp"

namespace chalet
{
struct MakefileGeneratorGNU final : IStrategyGenerator
{
	explicit MakefileGeneratorGNU(const BuildState& inState);

	virtual void addProjectRecipes(const ProjectConfiguration& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash) final;
	virtual std::string getContents(const std::string& inPath) const final;

private:
	std::string getBuildRecipes(const SourceOutputs& inOutputs);
	std::string getObjBuildRecipes(const StringList& inObjects, const std::string& pchTarget);
	std::string getAsmBuildRecipes(const StringList& inAssemblies);

	std::string getCompileEchoAsm(const std::string& assembly) const;
	std::string getCompileEchoSources(const std::string& source) const;
	std::string getCompileEchoLinker(const std::string& target) const;

	std::string getAsmRecipe(const std::string& object, const std::string& assembly) const;
	std::string getPchRecipe(const std::string& pchTarget);
	std::string getRcRecipe(const std::string& source, const std::string& object) const;
	std::string getCppRecipe(const std::string& source, const std::string& object, const std::string& pchTarget) const;
	std::string getObjcRecipe(const std::string& source, const std::string& object) const;

	std::string getTargetRecipe(const std::string& linkerTarget, const StringList& objects) const;
	std::string getDumpAsmRecipe(const StringList& inAssemblies) const;

	std::string getPchOrderOnlyPreReq() const;
	std::string getLinkerPreReqs(const StringList& objects) const;

	std::string getQuietFlag() const;
	std::string getMoveCommand(std::string inInput, std::string inOutput) const;
	std::string getPrinter(const std::string& inPrint = "", const bool inNewLine = false) const;

	std::string getColorBlue() const;
	std::string getColorPurple() const;

	StringList m_fileExtensions;
	StringList m_dependencies;
	StringList m_assemblies;
};
}

#endif // CHALET_MAKEFILE_GENERATOR_GNU_HPP
