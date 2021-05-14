/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/MetaStrategyNinja.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
MetaStrategyNinja::MetaStrategyNinja(BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
bool MetaStrategyNinja::createCache()
{
	auto& name = "ninja";
	std::string cacheFolder = m_state.cache.getHash(name, BuildCache::Type::Local);

	auto& environmentCache = m_state.cache.environmentCache();
	Json& buildCache = environmentCache.json["data"];
	std::string key = fmt::format("{}:{}", m_state.buildConfiguration(), name);

	std::string existingHash;
	if (buildCache.contains(key))
	{
		existingHash = buildCache.at(key);
	}

	m_cacheFile = fmt::format("{}/build.ninja", cacheFolder);
	const bool cacheExists = Commands::pathExists(cacheFolder) && Commands::pathExists(m_cacheFile);
	const bool appBuildChanged = m_state.cache.appBuildChanged();
	const auto hash = String::getPathFilename(cacheFolder);

	if (existingHash != hash || !cacheExists || appBuildChanged)
	{
		if (!cacheExists)
		{
			Commands::makeDirectory(cacheFolder);
		}

		// NinjaGenerator generator(m_state, m_project, m_toolchain);
		// std::ofstream(m_cacheFile) << generator.getContents(inOutputs, cacheFolder)
		// 						   << std::endl;

		buildCache[key] = String::getPathFilename(cacheFolder);
		m_state.cache.setDirty(true);
	}

	return true;
}
}
