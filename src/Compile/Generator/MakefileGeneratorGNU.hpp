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

#include <unordered_map>

namespace chalet
{
struct MakefileGeneratorGNU final : IStrategyGenerator
{
	explicit MakefileGeneratorGNU(const BuildState& inState);

	virtual void addProjectRecipes(const SourceTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash) final;
	virtual std::string getContents(const std::string& inPath) const final;

	virtual void reset() final;

private:
	std::string getBuildRecipes(const SourceOutputs& inOutputs);

	std::string getCompileEchoSources(const std::string& inFile = "$<") const;
	std::string getLinkerEcho() const;

	std::string getPchRecipe(const std::string& source, const std::string& object);
	std::string getRcRecipe(const std::string& ext, const std::string& pchTarget) const;
	std::string getCxxRecipe(const std::string& ext, const std::string& pchTarget) const;
	std::string getObjcRecipe(const std::string& ext) const;

	std::string getTargetRecipe(const std::string& linkerTarget) const;

	std::string getLinkerPreReqs() const;

	std::string getQuietFlag() const;
	std::string getFallbackMakeDependsCommand(const std::string& inDependencyFile, const std::string& object, const std::string& source) const;
	std::string getPrinter(const std::string& inPrint = "", const bool inNewLine = false) const;

	bool locationExists(const std::string& location, const std::string& ext) const;

	std::string getBuildColor() const;

	// StringList m_fileExtensions;

	mutable std::unordered_map<std::string, StringList> m_locationCache;
};
}

#endif // CHALET_MAKEFILE_GENERATOR_GNU_HPP
