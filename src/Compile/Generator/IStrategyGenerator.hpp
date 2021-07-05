/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ISTRATEGY_GENERATOR_HPP
#define CHALET_ISTRATEGY_GENERATOR_HPP

#include "Compile/Strategy/StrategyType.hpp"
#include "Compile/Toolchain/ICompileToolchain.hpp"
#include "State/BuildState.hpp"
#include "State/SourceOutputs.hpp"
#include "State/Target/ProjectTarget.hpp"

namespace chalet
{
struct IStrategyGenerator;
using StrategyGenerator = std::unique_ptr<IStrategyGenerator>;

struct IStrategyGenerator
{
	explicit IStrategyGenerator(const BuildState& inState);
	virtual ~IStrategyGenerator() = default;

	[[nodiscard]] static StrategyGenerator make(const StrategyType inType, BuildState& inState);

	virtual void addProjectRecipes(const ProjectTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash) = 0;
	virtual std::string getContents(const std::string& inPath) const = 0;
	virtual bool saveDependencies() const;
	virtual void reset();

	bool hasProjectRecipes() const;

protected:
	const BuildState& m_state;
	ICompileToolchain* m_toolchain = nullptr;
	const ProjectTarget* m_project = nullptr;

	StringList m_targetRecipes;
	StringList m_precompiledHeaders;

	std::string m_hash;

	bool m_generateDependencies = false;
};

}

#endif // CHALET_ISTRATEGY_GENERATOR_HPP
