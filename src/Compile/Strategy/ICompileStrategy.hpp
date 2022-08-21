/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ICOMPILE_STRATEGY_HPP
#define CHALET_ICOMPILE_STRATEGY_HPP

#include "Compile/CompileCommandsGenerator.hpp"
#include "Compile/Generator/IStrategyGenerator.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "State/SourceOutputs.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
class BuildState;
struct ICompileStrategy;
using CompileStrategy = Unique<ICompileStrategy>;

struct ICompileStrategy
{
	explicit ICompileStrategy(const StrategyType inType, BuildState& inState);
	virtual ~ICompileStrategy() = default;

	[[nodiscard]] static CompileStrategy make(const StrategyType inType, BuildState& inState);

	StrategyType type() const noexcept;
	bool isMSBuild() const noexcept;

	bool saveCompileCommands() const;

	void setSourceOutputs(const SourceTarget& inProject, Unique<SourceOutputs>&& inOutputs);
	void setToolchainController(const SourceTarget& inProject, CompileToolchain&& inToolchain);

	virtual bool initialize() = 0;
	virtual bool addProject(const SourceTarget& inProject);

	virtual bool doPreBuild();
	virtual bool doFullBuild();
	virtual bool buildProject(const SourceTarget& inProject) = 0;
	virtual bool doPostBuild() const;

	bool buildProjectModules(const SourceTarget& inProject);

protected:
	BuildState& m_state;

	Dictionary<Unique<SourceOutputs>> m_outputs;
	Dictionary<CompileToolchain> m_toolchains;

	StrategyGenerator m_generator;
	CompileCommandsGenerator m_compileCommandsGenerator;

	StrategyType m_type;

private:
	bool addCompileCommands(const SourceTarget& inProject);
};

}

#endif // CHALET_ICOMPILE_STRATEGY_HPP
