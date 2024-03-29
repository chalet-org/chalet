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

class NinjaGenerator final : public IStrategyGenerator
{
	using NinjaRule = std::function<std::string(NinjaGenerator&)>;
	using NinjaRuleList = std::unordered_map<SourceType, NinjaRule>;

public:
	explicit NinjaGenerator(const BuildState& inState);

	virtual void addProjectRecipes(const SourceTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash) final;
	virtual std::string getContents(const std::string& inPath) const final;

private:
	std::string getSafeNinjaPath(std::string path) const;
	std::string getDepFile(const std::string& inDependency);

	std::string getRules(const SourceOutputs& inOutputs);
	std::string getBuildRules(const SourceOutputs& inOutputs);

	std::string getDependencyRule();
	std::string getPchRule();
	std::string getRcRule();
	std::string getCRule();
	std::string getCppRule();
	std::string getObjcRule();
	std::string getObjcppRule();

	std::string getCxxRule(const std::string inId, const SourceType derivative);

	std::string getLinkRule();

	std::string getPchBuildRule(const std::string& pchTarget);
	std::string getObjBuildRules(const SourceFileGroupList& inGroups);

	std::string getRuleDeps() const;

	NinjaRuleList m_rules;

	bool m_needsMsvcDepsPrefix = false;
};
}
