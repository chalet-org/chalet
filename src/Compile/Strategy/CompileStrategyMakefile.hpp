/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_STRATEGY_MAKEFILE_HPP
#define CHALET_COMPILE_STRATEGY_MAKEFILE_HPP

#include "Compile/Strategy/ICompileStrategy.hpp"
#include "Compile/Toolchain/ICompileToolchain.hpp"

#include "BuildJson/ProjectConfiguration.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
struct CompileStrategyMakefile final : ICompileStrategy
{
	explicit CompileStrategyMakefile(BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain);

	virtual StrategyType type() const final;

	virtual bool createCache(const SourceOutputs& inOutputs) final;
	virtual bool initialize() final;
	virtual bool run() final;

private:
	BuildState& m_state;
	const ProjectConfiguration& m_project;
	CompileToolchain& m_toolchain;

	StringList m_makeCmd;

	std::string m_cacheFile;
};
}

#endif // CHALET_COMPILE_STRATEGY_MAKEFILE_HPP
