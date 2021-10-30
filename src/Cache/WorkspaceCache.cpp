/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Cache/WorkspaceCache.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildPaths.hpp"
#include "State/CompilerTools.hpp"
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
const std::string& WorkspaceCache::getCacheRef(const CacheType inCacheType) const
{
	// return inCacheType == Type::Global ? m_cacheFolderGlobal : m_cacheFolderLocal;
	UNUSED(inCacheType);
	return m_cacheFolderLocal;
}

/*****************************************************************************/
bool WorkspaceCache::initialize(const CommandLineInputs& inInputs)
{
	const auto& outputDirectory = inInputs.outputDirectory();
	m_cacheFolderLocal = fmt::format("{}/.cache", outputDirectory);
	// m_cacheFolderGlobal = fmt::format("{}/.chalet", m_inputs.homeDirectory());

	if (!m_cacheFile.initialize(getHashPath("chalet_workspace_file", CacheType::Local), inInputs.inputFile()))
		return false;

	if (!m_cacheFile.save())
	{
		Diagnostic::error("There was an error saving the workspace cache.");
		return false;
	}

	return true;
}

/*****************************************************************************/
bool WorkspaceCache::initializeSettings(const CommandLineInputs& inInputs)
{
	if (!m_localSettings.load(inInputs.settingsFile()))
		return false;

	if (!m_globalSettings.load(inInputs.getGlobalSettingsFilePath()))
		return false;

	m_removeOldCacheFolder = m_localSettings.json.empty();

	return true;
}

/*****************************************************************************/
bool WorkspaceCache::createCacheFolder(const CacheType inCacheType)
{
	const auto& cacheRef = getCacheRef(inCacheType);

	if (m_removeOldCacheFolder)
	{
		removeCacheFolder(inCacheType);
		m_removeOldCacheFolder = false;
	}

	Output::setShowCommandOverride(false);

	if (!Commands::pathExists(cacheRef))
	{
		if (!Commands::makeDirectory(cacheRef))
			return false;
	}

	Output::setShowCommandOverride(true);

	m_settingsCreated = true;

	return true;
}

/*****************************************************************************/
bool WorkspaceCache::settingsCreated() const noexcept
{
	return m_settingsCreated;
}

/*****************************************************************************/
bool WorkspaceCache::exists(const CacheType inCacheType) const
{
	if (inCacheType == CacheType::Local)
	{
		return Commands::pathExists(m_cacheFolderLocal) || Commands::pathExists(m_localSettings.filename());
	}
	else
	{
		return Commands::pathExists(m_globalSettings.filename());
	}
}

/*****************************************************************************/
void WorkspaceCache::removeCacheFolder(const CacheType inCacheType)
{
	const auto& cacheRef = getCacheRef(inCacheType);

	if (Commands::pathExists(cacheRef))
		Commands::removeRecursively(cacheRef);
}

/*****************************************************************************/
std::string WorkspaceCache::getHashPath(const std::string& inIdentifier, const CacheType inCacheType) const
{
	std::string hash = Hash::string(inIdentifier);

	const auto& cacheRef = getCacheRef(inCacheType);
	std::string ret = fmt::format("{}/{}", cacheRef, hash);

	// LOG(ret);

	return ret;
}

/*****************************************************************************/
std::string WorkspaceCache::getCachePath(const std::string& inIdentifier, const CacheType inCacheType) const
{
	const auto& cacheRef = getCacheRef(inCacheType);
	std::string ret;

	if (inIdentifier.empty())
		ret = cacheRef;
	else
		ret = fmt::format("{}/{}", cacheRef, inIdentifier);

	// LOG(ret);

	return ret;
}

/*****************************************************************************/
WorkspaceInternalCacheFile& WorkspaceCache::file() noexcept
{
	return m_cacheFile;
}

/*****************************************************************************/
JsonFile& WorkspaceCache::getSettings(const SettingsType inType) noexcept
{
	if (inType == SettingsType::Global)
		return m_globalSettings;
	else
		return m_localSettings;
}

const JsonFile& WorkspaceCache::getSettings(const SettingsType inType) const noexcept
{
	if (inType == SettingsType::Global)
		return m_globalSettings;
	else
		return m_localSettings;
}

/*****************************************************************************/
void WorkspaceCache::saveSettings(const SettingsType inType)
{
	if (inType == SettingsType::Global)
		m_globalSettings.save();
	else
		m_localSettings.save();
}

/*****************************************************************************/
bool WorkspaceCache::removeStaleProjectCaches()
{
	const auto& cacheRef = getCacheRef(CacheType::Local);
	StringList ids = m_cacheFile.getCacheIdsForRemoval();

	if (!Commands::pathExists(cacheRef) || ids.size() == 0)
		return true;

	Output::setShowCommandOverride(false);

	for (auto& id : ids)
	{
		auto path = fmt::format("{}/{}", cacheRef, id);
		if (!Commands::pathExists(path))
		{
			if (!m_cacheFile.removeSourceCache(id))
				m_cacheFile.removeExtraCache(id);
		}
	}

	bool result = true;

	CHALET_TRY
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
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR(err.what());
		result = false;
	}

	Output::setShowCommandOverride(true);

	return result;
}

/*****************************************************************************/
bool WorkspaceCache::saveProjectCache(const CommandLineInputs& inInputs)
{
	bool result = m_cacheFile.save();

	const auto& cacheRef = getCacheRef(CacheType::Local);
	const auto& outputDirectory = inInputs.outputDirectory();

	auto removePathIfEmpty = [](const std::string& inPath) {
		if (Commands::pathIsEmpty(inPath, {}, true))
			Commands::removeRecursively(inPath);
	};

	removePathIfEmpty(cacheRef);
	removePathIfEmpty(outputDirectory);

	if (!result)
	{
		Diagnostic::error("There was an error saving the workspace cache.");
		return false;
	}

	return true;
}

/*****************************************************************************/
bool WorkspaceCache::updateSettingsFromToolchain(const CommandLineInputs& inInputs, const CompilerTools& inToolchain)
{
	auto& settingsJson = getSettings(SettingsType::Local);

	const auto& settingsFile = inInputs.settingsFile();
	const auto& preference = inInputs.toolchainPreferenceName();
	const std::string kKeyOptions{ "options" };
	const std::string kKeyToolchains{ "toolchains" };

	if (!settingsJson.json.contains(kKeyOptions))
	{
		Diagnostic::error("{}: '{}' did not correctly initialize.", settingsFile, kKeyOptions);
		return false;
	}
	if (!settingsJson.json.contains(kKeyToolchains))
	{
		Diagnostic::error("{}: '{}' did not correctly initialize.", settingsFile, kKeyToolchains);
		return false;
	}

	auto& toolchains = settingsJson.json.at(kKeyToolchains);
	if (!toolchains.contains(preference))
	{
		Diagnostic::error("{}: '{}' did not correctly initialize.", settingsFile, kKeyToolchains);
		return false;
	}

	auto& optionsJson = settingsJson.json.at(kKeyOptions);
	auto& toolchain = toolchains.at(preference);

	const std::string kKeyToolchain{ "toolchain" };
	if (optionsJson.contains(kKeyToolchain))
	{
		auto& toolchainSetting = optionsJson.at(kKeyToolchain);
		if (toolchainSetting.is_string() && toolchainSetting.get<std::string>() != preference)
		{
			optionsJson[kKeyToolchain] = preference;
			settingsJson.setDirty(true);
		}
	}

	const std::string kKeyArchitecture{ "architecture" };
	if (optionsJson.contains(kKeyArchitecture))
	{
		std::string archString = inInputs.targetArchitecture().empty() ? "auto" : inInputs.targetArchitecture();
		auto& arch = optionsJson.at(kKeyArchitecture);
		if (arch.is_string() && arch.get<std::string>() != archString)
		{
			optionsJson[kKeyArchitecture] = archString;
			settingsJson.setDirty(true);
		}
	}

	const std::string kKeyVersion{ "version" };
	if (toolchain.contains(kKeyVersion))
	{
		const auto& versionString = inToolchain.version();
		auto& version = toolchain.at(kKeyVersion);
		if (version.is_string() && version.get<std::string>() != versionString)
		{
			toolchain[kKeyVersion] = versionString;
			settingsJson.setDirty(true);
		}
	}

	return true;
}

}
