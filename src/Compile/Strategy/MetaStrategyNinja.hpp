/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_META_STRATEGY_NINJA_HPP
#define CHALET_META_STRATEGY_NINJA_HPP

#include "Compile/Strategy/NinjaGenerator.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
class MetaStrategyNinja
{
public:
	explicit MetaStrategyNinja(BuildState& inState);

	bool initialize();

	bool addProject(const ProjectConfiguration& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain);
	bool saveBuildFile() const;

	bool buildProject(const ProjectConfiguration& inProject);

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
