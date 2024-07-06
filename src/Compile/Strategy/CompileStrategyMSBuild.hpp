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

	virtual std::string name() const noexcept final;
	virtual bool initialize() final;
	virtual bool addProject(const SourceTarget& inProject) final;

	virtual bool doFullBuild() final;
	virtual bool buildProject(const SourceTarget& inProject) final;

private:
	StringList getMsBuildCommand(const std::string& inMsBuild, const std::string& inProjectName) const;
	std::string getMsBuildTarget() const;
	bool subprocessMsBuild(const StringList& inCmd, std::string inCwd) const;

	std::string m_solution;

	bool m_initialized = false;
};
}
