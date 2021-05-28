/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ISTRATEGY_GENERATOR_HPP
#define CHALET_ISTRATEGY_GENERATOR_HPP

#include "BuildJson/Target/ProjectTarget.hpp"
#include "Compile/Toolchain/ICompileToolchain.hpp"
#include "State/BuildState.hpp"
#include "State/SourceOutputs.hpp"

namespace chalet
{
struct IStrategyGenerator
{
	explicit IStrategyGenerator(const BuildState& inState);
	virtual ~IStrategyGenerator() = default;

	virtual void addProjectRecipes(const ProjectTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash) = 0;
	virtual std::string getContents(const std::string& inPath) const = 0;

	bool hasProjectRecipes() const;

protected:
	const BuildState& m_state;
	ICompileToolchain* m_toolchain = nullptr;
	const ProjectTarget* m_project = nullptr;

	StringList m_targetRecipes;
	StringList m_precompiledHeaders;

	std::string m_hash;

	bool m_cleanOutput = false;
	bool m_generateDependencies = false;
};

using StrategyGenerator = std::unique_ptr<IStrategyGenerator>;
}

#endif // CHALET_ISTRATEGY_GENERATOR_HPP
