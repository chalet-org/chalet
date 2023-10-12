/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_STRATEGY_XCODEBUILD_HPP
#define CHALET_COMPILE_STRATEGY_XCODEBUILD_HPP

#include "Compile/Strategy/ICompileStrategy.hpp"

namespace chalet
{
class BuildState;

struct CompileStrategyXcodeBuild final : ICompileStrategy
{
	explicit CompileStrategyXcodeBuild(BuildState& inState);

	virtual bool initialize() final;
	virtual bool addProject(const SourceTarget& inProject) final;

	virtual bool doFullBuild() final;
	virtual bool buildProject(const SourceTarget& inProject) final;

private:
	bool m_initialized = false;
};
}

#endif // CHALET_COMPILE_STRATEGY_XCODEBUILD_HPP
