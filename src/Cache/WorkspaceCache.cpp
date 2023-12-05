/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Cache/WorkspaceCache.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Platform/Arch.hpp"
#include "Process/Environment.hpp"
#include "SettingsJson/ThemeSettingsJsonParser.hpp"
#include "State/BuildPaths.hpp"
#include "State/CentralState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonKeys.hpp"
#include "Json/JsonValues.hpp"

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

	if (!m_cacheFile.initialize(getHashPath("chalet_workspace_file"), inInputs.inputFile()))
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
	auto globalSettingsFile = inInputs.getGlobalSettingsFilePath();
	auto globalSettingsFolder = String::getPathFolder(globalSettingsFile);

	if (!Files::pathExists(globalSettingsFolder))
		Files::makeDirectory(globalSettingsFolder);

	// Migrate old settings path pre 0.5.0
	auto oldGlobalSettingsFile = fmt::format("{}/.chaletconfig", inInputs.homeDirectory());
	if (Files::pathExists(oldGlobalSettingsFile))
	{
		if (Files::moveSilent(oldGlobalSettingsFile, globalSettingsFile))
		{
			// Update the theme
			ThemeSettingsJsonParser themeParser(inInputs);
			// we don't care if this succeeded or not. we care about initializing settings below
			themeParser.serialize();
		}
	}

	if (!m_globalSettings.load(globalSettingsFile))
		return false;

	if (!m_localSettings.load(inInputs.settingsFile()))
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

	if (!Files::pathExists(cacheRef))
	{
		if (!Files::makeDirectory(cacheRef))
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
		return Files::pathExists(m_cacheFolderLocal) || Files::pathExists(m_localSettings.filename());
	}
	else
	{
		return Files::pathExists(m_globalSettings.filename());
	}
}

/*****************************************************************************/
void WorkspaceCache::removeCacheFolder(const CacheType inCacheType)
{
	const auto& cacheRef = getCacheRef(inCacheType);

	if (Files::pathExists(cacheRef))
		Files::removeRecursively(cacheRef);
}

/*****************************************************************************/
std::string WorkspaceCache::getHashPath(const std::string& inIdentifier) const
{
	std::string hash = Hash::string(inIdentifier);

	const auto& cacheRef = getCacheRef(CacheType::Local);
	std::string ret = fmt::format("{}/{}", cacheRef, hash);

	// LOG(ret);

	return ret;
}

/*****************************************************************************/
std::string WorkspaceCache::getCachePath(const std::string& inIdentifier) const
{
	const auto& cacheRef = getCacheRef(CacheType::Local);
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
	StringList ids = m_cacheFile.getCacheIdsToNotRemove();

	if (!Files::pathExists(cacheRef) || ids.empty())
		return true;

	Output::setShowCommandOverride(false);

	for (auto& id : ids)
	{
		auto path = fmt::format("{}/{}", cacheRef, id);
		if (!Files::pathExists(path))
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
					result &= Files::removeRecursively(path.string());
			}
			else if (it.is_regular_file())
			{
				const auto& path = it.path();
				const auto filename = path.filename().string();

				if (!List::contains(ids, filename))
					result &= Files::removeIfExists(path.string());
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
		if (Files::pathIsEmpty(inPath))
			Files::removeRecursively(inPath);
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
bool WorkspaceCache::updateSettingsFromToolchain(const CommandLineInputs& inInputs, const CentralState& inCentralState, const CompilerTools& inToolchain)
{
	auto& settingsJson = getSettings(SettingsType::Local);
	auto& globalSettingsJson = getSettings(SettingsType::Global);

	const auto& settingsFile = settingsJson.filename();
	const auto& globalSettingsFile = globalSettingsJson.filename();
	const auto& preference = inInputs.toolchainPreferenceName();
	const auto& arch = inInputs.getResolvedTargetArchitecture();

	if (!settingsJson.json.contains(Keys::Options))
	{
		Diagnostic::error("{}: '{}' did not correctly initialize.", settingsFile, Keys::Options);
		return false;
	}
	if (!globalSettingsJson.json.contains(Keys::Options))
	{
		Diagnostic::error("{}: '{}' did not correctly initialize.", globalSettingsFile, Keys::Options);
		return false;
	}
	if (!settingsJson.json.contains(Keys::Toolchains))
	{
		Diagnostic::error("{}: '{}' did not correctly initialize.", settingsFile, Keys::Toolchains);
		return false;
	}
	if (!globalSettingsJson.json.contains(Keys::Toolchains))
	{
		Diagnostic::error("{}: '{}' did not correctly initialize.", globalSettingsFile, Keys::Toolchains);
		return false;
	}

	auto& toolchains = settingsJson.json.at(Keys::Toolchains);
	if (!toolchains.contains(preference))
	{
		Diagnostic::error("{}: '{}' did not correctly initialize.", settingsFile, Keys::Toolchains);
		return false;
	}

	auto fetchToolchain = [&toolchains, &preference, &arch]() -> Json& {
		auto arch2 = Arch::from(arch);
		auto& rootToolchain = toolchains.at(preference);
		if (rootToolchain.contains(arch2.str))
			return rootToolchain.at(arch2.str);
		else
			return rootToolchain;
	};

	auto& optionsJson = settingsJson.json.at(Keys::Options);
	if (optionsJson.contains(Keys::OptionsToolchain))
	{
		auto& toolchainSetting = optionsJson.at(Keys::OptionsToolchain);
		if (toolchainSetting.is_string() && toolchainSetting.get<std::string>() != preference)
		{
			optionsJson[Keys::OptionsToolchain] = preference;
			settingsJson.setDirty(true);
		}
	}

	if (optionsJson.contains(Keys::OptionsArchitecture))
	{
		std::string archString{ Values::Auto };
		if (!inInputs.targetArchitecture().empty())
		{
			if (String::equals("gcc", preference))
			{
				auto arch2 = Arch::from(arch);
				archString = arch2.str;
			}
			else
			{
				archString = inInputs.targetArchitecture();
			}
		}
		auto& archJson = optionsJson.at(Keys::OptionsArchitecture);
		if (archJson.is_string() && archJson.get<std::string>() != archString)
		{
			optionsJson[Keys::OptionsArchitecture] = archString;
			settingsJson.setDirty(true);
		}
	}

	if (optionsJson.contains(Keys::OptionsLastTarget))
	{
		auto& lastTarget = inInputs.lastTarget();
		auto& lastTargetNode = optionsJson.at(Keys::OptionsLastTarget);
		if (lastTargetNode.is_string() && lastTargetNode.get<std::string>() != lastTarget)
		{
			optionsJson[Keys::OptionsLastTarget] = lastTarget;
			settingsJson.setDirty(true);
		}
	}

	if (optionsJson.contains(Keys::OptionsRunArguments))
	{
		auto& runArgsJson = optionsJson.at(Keys::OptionsRunArguments);
		if (runArgsJson.is_object())
		{
			const auto& argumentMap = inCentralState.runArgumentMap();
			for (const auto& [key, value] : argumentMap)
			{
				if (runArgsJson.find(key) != runArgsJson.end())
				{
					auto& existing = runArgsJson.at(key);
					if (existing.is_string() && existing.get<std::string>() != value)
					{
						runArgsJson[key] = value;
						settingsJson.setDirty(true);
					}
				}
				else
				{
					runArgsJson[key] = value;
					settingsJson.setDirty(true);
				}
			}
		}
	}

	auto& toolchain = fetchToolchain();
	if (toolchain.contains(Keys::ToolchainVersion))
	{
		const auto& versionString = inToolchain.version();
		auto& version = toolchain.at(Keys::ToolchainVersion);
		if (version.is_string() && version.get<std::string>() != versionString)
		{
			toolchain[Keys::ToolchainVersion] = versionString;
			settingsJson.setDirty(true);
		}
	}

	if (inInputs.saveUserToolchainGlobally())
	{
		auto& globalToolchains = globalSettingsJson.json.at(Keys::Toolchains);
		globalToolchains[preference] = toolchain;

		auto& globalOptionsJson = settingsJson.json.at(Keys::Options);
		globalOptionsJson[Keys::OptionsToolchain] = preference;

		globalSettingsJson.setDirty(true);
	}

	return true;
}

}
