/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Generator/IStrategyGenerator.hpp"
#include "State/SourceOutputs.hpp"

namespace chalet
{
class BuildState;
struct MakefileGeneratorGNU final : IStrategyGenerator
{
	explicit MakefileGeneratorGNU(const BuildState& inState);

	virtual void addProjectRecipes(const SourceTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash) final;
	virtual std::string getContents(const std::string& inPath) const final;

	virtual void reset() final;

private:
	std::string getBuildRecipes(const SourceOutputs& inOutputs);
	std::string getObjBuildRecipes(const SourceFileGroupList& inGroups);

	std::string getCompileEchoSources(const std::string& inFile) const;
	std::string getLinkerEcho(const std::string& inFile) const;

	std::string getPchRecipe(const std::string& source, const std::string& object, const std::string& dependency);
	std::string getRcRecipe(const std::string& source, const std::string& object, const std::string& dependency) const;
	std::string getCxxRecipe(const std::string& source, const std::string& object, const std::string& dependency, const std::string& pchTarget, const SourceType derivative) const;

	std::string getTargetRecipe(const std::string& linkerTarget, const StringList& objects) const;

	std::string getLinkerPreReqs(const StringList& objects) const;

	std::string getQuietFlag() const;
	std::string getFallbackMakeDependsCommand(const std::string& inDependencyFile, const std::string& object, const std::string& source) const;
	std::string getPrinter(const std::string& inPrint = "", const bool inNewLine = false) const;

	size_t m_fileCount = 0;
};
}
