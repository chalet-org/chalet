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
	using NinjaRuleList = std::map<std::string, NinjaRule>;

public:
	explicit NinjaGenerator(const BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain);

	std::string getContents(const SourceOutputs& inOutputs, const std::string& cacheDir);

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
	const ProjectConfiguration& m_project;
	CompileToolchain& m_toolchain;

	NinjaRuleList m_rules;
};
}

#endif // CHALET_NINJA_GENERATOR_HPP
