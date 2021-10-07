/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ICOMPILE_STRATEGY_HPP
#define CHALET_ICOMPILE_STRATEGY_HPP

#include "Compile/CompileCommandsGenerator.hpp"
#include "Compile/Generator/IStrategyGenerator.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "Compile/Toolchain/ICompileToolchain.hpp"
#include "State/BuildState.hpp"
#include "State/SourceOutputs.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
struct ICompileStrategy;
using CompileStrategy = std::unique_ptr<ICompileStrategy>;

struct ICompileStrategy
{
	explicit ICompileStrategy(const StrategyType inType, BuildState& inState);
	virtual ~ICompileStrategy() = default;

	[[nodiscard]] static CompileStrategy make(const StrategyType inType, BuildState& inState);

	StrategyType type() const noexcept;

	bool addCompileCommands(const SourceTarget& inProject, CompileToolchain& inToolchain);
	bool saveCompileCommands() const;

	const SourceOutputs& getSourceOutput(const std::string& inTarget);

	virtual bool initialize(const StringList& inFileExtensions) = 0;
	virtual bool addProject(const SourceTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain) = 0;

	virtual bool saveBuildFile() const = 0;
	virtual bool buildProject(const SourceTarget& inProject) const = 0;
	virtual bool doPostBuild() const;

protected:
	BuildState& m_state;

	Dictionary<SourceOutputs> m_outputs;

	StrategyGenerator m_generator;
	CompileCommandsGenerator m_compileCommandsGenerator;

	StrategyType m_type;
};

}

#endif // CHALET_ICOMPILE_STRATEGY_HPP
