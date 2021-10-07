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
	virtual bool addProject(const SourceTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain) final;

	virtual bool saveBuildFile() const final;
	virtual bool buildProject(const SourceTarget& inProject) const final;

private:
	bool buildMake(const SourceTarget& inProject) const;
#if defined(CHALET_WIN32)
	bool buildNMake(const SourceTarget& inProject) const;
#endif

	bool subprocessMakefile(const StringList& inCmd, std::string inCwd = std::string()) const;

	std::string m_cacheFolder;

	Dictionary<std::string> m_hashes;
	Dictionary<std::string> m_buildFiles;

	bool m_initialized = false;
	bool m_cacheNeedsUpdate = false;
};
}

#endif // CHALET_COMPILE_STRATEGY_MAKEFILE_HPP
