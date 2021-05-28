/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_NINJA_GENERATOR_HPP
#define CHALET_NINJA_GENERATOR_HPP

#include "Compile/Generator/IStrategyGenerator.hpp"
#include "Compile/Toolchain/ICompileToolchain.hpp"
#include "State/BuildState.hpp"
#include "State/SourceOutputs.hpp"

namespace chalet
{
class NinjaGenerator final : public IStrategyGenerator
{
	using NinjaRule = std::function<std::string(NinjaGenerator&)>;
	using NinjaRuleList = std::unordered_map<std::string, NinjaRule>;

public:
	explicit NinjaGenerator(const BuildState& inState);

	virtual void addProjectRecipes(const ProjectTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash) final;
	virtual std::string getContents(const std::string& inPath) const final;

private:
	std::string getDepFile(const std::string& inDependency);

	std::string getRules(const StringList& inExtensions);
	std::string getBuildRules(const SourceOutputs& inOutputs);

	std::string getDependencyRule();
	std::string getPchRule();
	std::string getRcRule();
	std::string getAsmRule();
	std::string getCppRule();
	std::string getObjcRule();
	std::string getObjcppRule();

	std::string getLinkRule();

	std::string getPchBuildRule(const std::string& pchTarget);
	std::string getObjBuildRules(const StringList& inObjects, const std::string& pchTarget);
	std::string getAsmBuildRules(const StringList& inAssemblies);

	std::string getRuleDeps() const;

	NinjaRuleList m_rules;
	StringList m_assemblies;

	bool m_needsMsvcDepsPrefix = false;
};
}

#endif // CHALET_NINJA_GENERATOR_HPP
