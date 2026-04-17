/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Cache/WorkspaceInternalCacheFile.hpp"
#include "Settings/SettingsType.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct CommandLineInputs;
struct CentralState;
struct CompilerTools;
class BuildState;

struct WorkspaceCache
{
	WorkspaceCache() = default;

	bool createLocalCacheFolder();
	bool exists() const;
	std::string getHashPath(const std::string& inIdentifier) const;
	std::string getCachePath(const std::string& inIdentifier) const;

	WorkspaceInternalCacheFile& file() noexcept;

	JsonFile& getSettings(const SettingsType inType) noexcept;
	const JsonFile& getSettings(const SettingsType inType) const noexcept;
	void saveSettings(const SettingsType inType);

	bool removeStaleProjectCaches();
	bool saveProjectCache(const CommandLineInputs& inInputs);
	bool settingsCreated() const noexcept;

private:
	friend class BuildState;
	friend struct CentralState;
	friend struct SettingsManager;

	void removeLocalCacheFolder();

	bool initialize(const CommandLineInputs& inInputs);
	bool initializeSettings(const CommandLineInputs& inInputs);

	bool updateSettingsFromToolchain(const CommandLineInputs& inInputs, const CentralState& inCentralState, const CompilerTools& inToolchain);

	WorkspaceInternalCacheFile m_cacheFile;

	JsonFile m_localSettings;
	JsonFile m_globalSettings;

	std::string m_cacheFolderLocal;

	bool m_settingsCreated = false;
	bool m_removeOldCacheFolder = false;
};
}
