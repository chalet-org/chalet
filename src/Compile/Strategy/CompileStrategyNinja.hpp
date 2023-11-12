/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

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

	virtual bool doPreBuild() final;
	virtual bool buildProject(const SourceTarget& inProject) final;

private:
	std::string m_cacheFile;
	std::string m_cacheFolder;

	Dictionary<std::string> m_hashes;

	bool m_initialized = false;
	bool m_cacheNeedsUpdate = false;
};
}
