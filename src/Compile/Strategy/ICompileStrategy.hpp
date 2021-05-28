/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ICOMPILE_STRATEGY_HPP
#define CHALET_ICOMPILE_STRATEGY_HPP

#include "State/Target/ProjectTarget.hpp"
#include "Compile/Generator/IStrategyGenerator.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "Compile/Toolchain/ICompileToolchain.hpp"
#include "State/BuildState.hpp"
#include "State/SourceOutputs.hpp"

namespace chalet
{
struct ICompileStrategy
{
	explicit ICompileStrategy(const StrategyType inType, BuildState& inState);
	virtual ~ICompileStrategy() = default;

	StrategyType type() const noexcept;

	virtual bool initialize() = 0;
	virtual bool addProject(const ProjectTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain) = 0;

	virtual bool saveBuildFile() const = 0;
	virtual bool buildProject(const ProjectTarget& inProject) const = 0;

protected:
	BuildState& m_state;

	std::unordered_map<std::string, SourceOutputs> m_outputs;

	StrategyGenerator m_generator;

	StrategyType m_type;
};

using CompileStrategy = std::unique_ptr<ICompileStrategy>;
}

#endif // CHALET_ICOMPILE_STRATEGY_HPP
