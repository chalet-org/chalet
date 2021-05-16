/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_NINJA_GENERATOR_HPP
#define CHALET_NINJA_GENERATOR_HPP

#include "Compile/Toolchain/ICompileToolchain.hpp"
#include "State/BuildState.hpp"
#include "State/SourceOutputs.hpp"

namespace chalet
{
class NinjaGenerator
{
	using NinjaRule = std::function<std::string(NinjaGenerator&)>;
	using NinjaRuleList = std::unordered_map<std::string, NinjaRule>;

public:
	explicit NinjaGenerator(const BuildState& inState);

	void addProjectRecipes(const ProjectConfiguration& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash);

	bool hasProjectRecipes() const;

	std::string getContents(const std::string& cacheDir) const;

private:
	std::string getMoveCommand();

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

	const BuildState& m_state;
	ICompileToolchain* m_toolchain = nullptr;
	const ProjectConfiguration* m_project = nullptr;

	StringList m_targetRecipes;
	StringList m_precompiledHeaders;

	NinjaRuleList m_rules;

	std::string m_hash;

	bool m_generateDependencies = false;
};
}

#endif // CHALET_NINJA_GENERATOR_HPP
