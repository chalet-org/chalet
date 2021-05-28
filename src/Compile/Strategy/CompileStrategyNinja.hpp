/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_STRATEGY_NINJA_HPP
#define CHALET_COMPILE_STRATEGY_NINJA_HPP

#include "Compile/Generator/NinjaGenerator.hpp"
#include "Compile/Strategy/ICompileStrategy.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
struct CompileStrategyNinja final : ICompileStrategy
{
	explicit CompileStrategyNinja(BuildState& inState);

	virtual bool initialize() final;
	virtual bool addProject(const ProjectTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain) final;

	virtual bool saveBuildFile() const final;
	virtual bool buildProject(const ProjectTarget& inProject) const final;

private:
	bool subprocessNinja(const StringList& inCmd, const bool inCleanOutput, std::string inCwd = std::string()) const;

	std::string m_cacheFile;
	std::string m_cacheFolder;

	std::unordered_map<std::string, std::string> m_hashes;

	bool m_initialized = false;
	bool m_cacheNeedsUpdate = false;
};
}

#endif // CHALET_COMPILE_STRATEGY_NINJA_HPP
