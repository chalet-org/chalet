/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MAKEFILE_GENERATOR_NMAKE_HPP
#define CHALET_MAKEFILE_GENERATOR_NMAKE_HPP

#include "Compile/Generator/IStrategyGenerator.hpp"
#include "Compile/Toolchain/ICompileToolchain.hpp"
#include "State/BuildState.hpp"
#include "State/SourceOutputs.hpp"

namespace chalet
{
#if defined(CHALET_WIN32)
struct MakefileGeneratorNMake final : IStrategyGenerator
{
	explicit MakefileGeneratorNMake(const BuildState& inState);

	virtual void addProjectRecipes(const ProjectTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash) final;
	virtual std::string getContents(const std::string& inPath) const final;

private:
	std::string getBuildRecipes(const SourceOutputs& inOutputs);
	std::string getObjBuildRecipes(const StringList& inObjects, const std::string& pchTarget);
	std::string getAsmBuildRecipes(const StringList& inAssemblies);

	std::string getCompileEchoAsm(const std::string& assembly) const;
	std::string getCompileEchoSources(const std::string& source) const;
	std::string getCompileEchoLinker(const std::string& target) const;

	std::string getPchBuildRecipe(const std::string& pchTarget) const;

	std::string getAsmRecipe(const std::string& object, const std::string& assembly) const;
	std::string getPchRecipe(const std::string& pchTarget);
	std::string getRcRecipe(const std::string& source, const std::string& object) const;
	std::string getCppRecipe(const std::string& source, const std::string& object, const std::string& pchTarget) const;

	std::string getTargetRecipe(const std::string& linkerTarget, const StringList& objects) const;
	std::string getDumpAsmRecipe(const StringList& inAssemblies) const;

	std::string getLinkerPreReqs(const StringList& objects) const;

	std::string getQuietFlag() const;
	std::string getPrinter(const std::string& inPrint = "") const;

	std::string getColorBlue() const;
	std::string getColorPurple() const;

	StringList m_fileExtensions;
	// StringList m_dependencies;
	StringList m_assemblies;
};
#else
struct MakefileGeneratorNMake
{
	MakefileGeneratorNMake();
};
#endif
}

#endif // CHALET_MAKEFILE_GENERATOR_NMAKE_HPP
