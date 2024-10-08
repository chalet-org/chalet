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

struct MakefileGeneratorNMake final : IStrategyGenerator
{
	explicit MakefileGeneratorNMake(const BuildState& inState);

	virtual void addProjectRecipes(const SourceTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash) final;
	virtual std::string getContents(const std::string& inPath) const final;

	virtual void reset() override;

private:
	std::string getBuildRecipes(const SourceOutputs& inOutputs);
	std::string getObjBuildRecipes(const SourceFileGroupList& inGroups);

	std::string getCompileEcho(const std::string& source) const;
	std::string getLinkerEcho(const std::string& target) const;

	std::string getPchBuildRecipe(const StringList& inPches) const;

	std::string getPchRecipe(const std::string& source, const std::string& object);
	std::string getRcRecipe(const std::string& source, const std::string& object) const;
	std::string getCxxRecipe(const std::string& source, const std::string& object, const std::string& pchTarget, const SourceType derivative) const;

	std::string getTargetRecipe(const std::string& linkerTarget, const StringList& objects) const;

	std::string getLinkerPreReqs(const StringList& objects) const;

	std::string getQuietFlag() const;
	std::string getPrinter(const std::string& inPrint = "") const;
};
}
