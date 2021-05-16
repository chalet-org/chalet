/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_META_STRATEGY_NINJA_HPP
#define CHALET_META_STRATEGY_NINJA_HPP

#include "Compile/Strategy/IMetaStrategy.hpp"
#include "Compile/Strategy/NinjaGenerator.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
struct MetaStrategyNinja final : IMetaStrategy
{
	explicit MetaStrategyNinja(BuildState& inState);

	virtual StrategyType type() const noexcept final;

	virtual bool initialize() final;
	virtual bool addProject(const ProjectConfiguration& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain) final;

	virtual bool saveBuildFile() const final;
	virtual bool buildProject(const ProjectConfiguration& inProject) const final;

private:
	BuildState& m_state;

	NinjaGenerator m_generator;

	std::string m_cacheFile;
	std::string m_cacheFolder;

	std::unordered_map<std::string, std::string> m_hashes;

	bool m_initialized = false;
	bool m_cacheNeedsUpdate = false;
};
}

#endif // CHALET_META_STRATEGY_NINJA_HPP
