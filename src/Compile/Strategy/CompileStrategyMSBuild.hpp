/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Strategy/ICompileStrategy.hpp"

namespace chalet
{
class BuildState;

struct CompileStrategyMSBuild final : ICompileStrategy
{
	explicit CompileStrategyMSBuild(BuildState& inState);

	virtual bool initialize() final;
	virtual bool addProject(const SourceTarget& inProject) final;

	virtual bool doFullBuild() final;
	virtual bool buildProject(const SourceTarget& inProject) final;

private:
	bool m_initialized = false;
};
}
