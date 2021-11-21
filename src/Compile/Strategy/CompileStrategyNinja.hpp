/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_STRATEGY_NINJA_HPP
#define CHALET_COMPILE_STRATEGY_NINJA_HPP

#include "Compile/Generator/NinjaGenerator.hpp"
#include "Compile/Strategy/ICompileStrategy.hpp"

namespace chalet
{
class BuildState;

struct CompileStrategyNinja final : ICompileStrategy
{
	explicit CompileStrategyNinja(BuildState& inState);

	virtual bool initialize() final;
	virtual bool addProject(const SourceTarget& inProject) final;

	virtual bool saveBuildFile() const final;
	virtual bool buildProject(const SourceTarget& inProject) final;

private:
	std::string m_cacheFile;
	std::string m_cacheFolder;

	Dictionary<std::string> m_hashes;

	bool m_initialized = false;
	bool m_cacheNeedsUpdate = false;
};
}

#endif // CHALET_COMPILE_STRATEGY_NINJA_HPP
