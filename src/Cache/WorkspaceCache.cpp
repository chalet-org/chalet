/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Cache/WorkspaceCache.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
WorkspaceCache::WorkspaceCache(const CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
}

/*****************************************************************************/
const std::string& WorkspaceCache::getCacheRef(const Type inCacheType) const
{
	// return inCacheType == Type::Global ? m_cacheFolderGlobal : m_cacheFolderLocal;
	UNUSED(inCacheType);
	return m_cacheFolderLocal;
}

/*****************************************************************************/
bool WorkspaceCache::initialize()
{
	m_globalConfig.load(fmt::format("{}/.chaletconfig", m_inputs.homeDirectory()));
	m_localConfig.load(fmt::format("{}/chalet-cache.json", m_inputs.buildPath()));

	m_removeOldCacheFolder = m_localConfig.json.empty();

	// m_cacheFolderGlobal = fmt::format("{}/.chalet", m_inputs.homeDirectory());

	const auto& buildPath = m_inputs.buildPath();
	m_cacheFolderLocal = fmt::format("{}/.cache", buildPath);

	if (!m_cacheFile.initialize(getHashPath("chalet_workspace_file", Type::Local)))
		return false;

	if (!m_cacheFile.save())
	{
		Diagnostic::error("There was an error saving the workspace cache.");
		return false;
	}

	return true;
}

/*****************************************************************************/
bool WorkspaceCache::createCacheFolder(const Type inCacheType)
{
	const auto& cacheRef = getCacheRef(inCacheType);

	if (m_removeOldCacheFolder)
	{
		removeCacheFolder(inCacheType);
		m_removeOldCacheFolder = false;
	}

	Output::setShowCommandOverride(false);

	if (!Commands::pathExists(cacheRef))
		return Commands::makeDirectory(cacheRef);

	Output::setShowCommandOverride(true);

	return true;
}

/*****************************************************************************/
bool WorkspaceCache::exists(const Type inCacheType) const
{
	const auto& cacheRef = getCacheRef(inCacheType);
	return Commands::pathExists(cacheRef) || Commands::pathExists(m_localConfig.filename());
}

/*****************************************************************************/
void WorkspaceCache::removeCacheFolder(const Type inCacheType)
{
	const auto& cacheRef = getCacheRef(inCacheType);

	if (Commands::pathExists(cacheRef))
		Commands::removeRecursively(cacheRef);
}

/*****************************************************************************/
std::string WorkspaceCache::getHashPath(const std::string& inIdentifier, const Type inCacheType) const
{
	std::string toHash = fmt::format("{}_{}", m_workspaceHash, inIdentifier);
	std::string hash = Hash::string(toHash);

	const auto& cacheRef = getCacheRef(inCacheType);
	std::string ret = fmt::format("{}/{}", cacheRef, hash);

	// LOG(ret);

	return ret;
}

/*****************************************************************************/
std::string WorkspaceCache::getCachePath(const std::string& inFolder, const Type inCacheType) const
{
	const auto& cacheRef = getCacheRef(inCacheType);
	std::string ret;

	if (inFolder.empty())
		ret = cacheRef;
	else
		ret = fmt::format("{}/{}", cacheRef, inFolder);

	// LOG(ret);

	return ret;
}

/*****************************************************************************/
void WorkspaceCache::setWorkspaceHash(const std::string& inToHash) noexcept
{
	m_workspaceHash = Hash::uint64(inToHash);
}

/*****************************************************************************/
WorkspaceInternalCacheFile& WorkspaceCache::file() noexcept
{
	return m_cacheFile;
}

/*****************************************************************************/
JsonFile& WorkspaceCache::localConfig() noexcept
{
	return m_localConfig;
}

/*****************************************************************************/
void WorkspaceCache::saveLocalConfig()
{
	m_localConfig.save();
}

/*****************************************************************************/
JsonFile& WorkspaceCache::globalConfig() noexcept
{
	return m_globalConfig;
}

/*****************************************************************************/
void WorkspaceCache::saveGlobalConfig()
{
	m_globalConfig.save();
}

/*****************************************************************************/
void WorkspaceCache::removeBuildIfCacheChanged(const std::string& inBuildDir)
{
	if (!Commands::pathExists(inBuildDir))
		return;

	if (m_cacheFile.workingDirectoryChanged())
		Commands::removeRecursively(inBuildDir);
}

/*****************************************************************************/
bool WorkspaceCache::removeStaleProjectCaches()
{
	const auto& cacheRef = getCacheRef(Type::Local);
	StringList ids = m_cacheFile.getCacheIds();

	if (!Commands::pathExists(cacheRef) || ids.size() == 0)
		return true;

	for (auto& id : ids)
	{
		auto path = fmt::format("{}/{}", cacheRef, id);
		if (!Commands::pathExists(path))
		{
			m_cacheFile.removeSourceCache(id);
		}
	}

	bool result = true;

	try
	{
		for (auto& it : fs::directory_iterator(cacheRef))
		{
			if (it.is_directory())
			{
				const auto& path = it.path();
				const auto stem = path.stem().string();

				if (!List::contains(ids, stem))
					result &= Commands::removeRecursively(path.string());
			}
			else if (it.is_regular_file())
			{
				const auto& path = it.path();
				const auto filename = path.filename().string();

				if (!List::contains(ids, filename))
					result &= Commands::remove(path.string());
			}
		}
	}
	catch (const std::exception& err)
	{
		Diagnostic::error(err.what());
		result = false;
	}

	return result;
}

}
