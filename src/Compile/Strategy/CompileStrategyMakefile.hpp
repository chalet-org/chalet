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

	virtual bool initialize(const StringList& inFileExtensions) final;
	virtual bool addProject(const ProjectTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain) final;

	virtual bool saveBuildFile() const final;
	virtual bool buildProject(const ProjectTarget& inProject) const final;
	virtual bool doPostBuild() const final;

private:
	bool buildMake(const ProjectTarget& inProject) const;
#if defined(CHALET_WIN32)
	bool buildNMake(const ProjectTarget& inProject) const;
#endif

	bool subprocessMakefile(const StringList& inCmd, std::string inCwd = std::string()) const;

	std::string m_cacheFolder;

	std::unordered_map<std::string, std::string> m_hashes;
	std::unordered_map<std::string, std::string> m_buildFiles;

	bool m_initialized = false;
	bool m_cacheNeedsUpdate = false;
};
}

#endif // CHALET_COMPILE_STRATEGY_MAKEFILE_HPP
