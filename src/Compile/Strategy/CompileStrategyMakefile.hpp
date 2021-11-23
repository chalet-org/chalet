/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_STRATEGY_MAKEFILE_HPP
#define CHALET_COMPILE_STRATEGY_MAKEFILE_HPP

#include "Compile/Generator/IStrategyGenerator.hpp"
#include "Compile/Strategy/ICompileStrategy.hpp"

namespace chalet
{
class BuildState;

struct CompileStrategyMakefile final : ICompileStrategy
{
	explicit CompileStrategyMakefile(BuildState& inState);

	virtual bool initialize() final;
	virtual bool addProject(const SourceTarget& inProject) final;

	virtual bool doPreBuild() final;
	virtual bool buildProject(const SourceTarget& inProject) final;

	virtual bool doPostBuild() const final;

private:
	bool buildMake(const SourceTarget& inProject) const;
#if defined(CHALET_WIN32)
	bool buildNMake(const SourceTarget& inProject) const;
#endif

	bool subprocessMakefile(const StringList& inCmd, std::string inCwd = std::string()) const;
	void setBuildEnvironment(const SourceOutputs& inOutput, const std::string& inHash) const;

	std::string m_cacheFolder;

	Dictionary<std::string> m_hashes;
	Dictionary<std::string> m_buildFiles;

	bool m_initialized = false;
	bool m_cacheNeedsUpdate = false;
};
}

#endif // CHALET_COMPILE_STRATEGY_MAKEFILE_HPP
