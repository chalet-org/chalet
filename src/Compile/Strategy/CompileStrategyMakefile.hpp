/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_STRATEGY_MAKEFILE_HPP
#define CHALET_COMPILE_STRATEGY_MAKEFILE_HPP

#include "Compile/Generator/IStrategyGenerator.hpp"
#include "Compile/Strategy/ICompileStrategy.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
struct CompileStrategyMakefile final : ICompileStrategy
{
	explicit CompileStrategyMakefile(BuildState& inState);

	virtual bool initialize() final;
	virtual bool addProject(const ProjectConfiguration& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain) final;

	virtual bool saveBuildFile() const final;
	virtual bool buildProject(const ProjectConfiguration& inProject) const final;

private:
	bool buildMake(const ProjectConfiguration& inProject) const;
#if defined(CHALET_WIN32)
	bool buildNMake(const ProjectConfiguration& inProject) const;
#endif

	bool subprocessMakefile(const StringList& inCmd, const bool inCleanOutput, std::string inCwd = std::string()) const;

	std::string m_cacheFile;

	std::unordered_map<std::string, std::string> m_hashes;

	bool m_initialized = false;
	bool m_cacheNeedsUpdate = false;
};
}

#endif // CHALET_COMPILE_STRATEGY_MAKEFILE_HPP
