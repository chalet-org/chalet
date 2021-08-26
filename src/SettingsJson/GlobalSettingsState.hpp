/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_GLOBAL_SETTINGS_STATE_HPP
#define CHALET_GLOBAL_SETTINGS_STATE_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
struct GlobalSettingsState
{
	Json toolchains;
	Json tools;
	Json appleSdks;

	std::string buildConfiguration;
	std::string toolchainPreference;
	std::string architecturePreference;
	std::string inputFile;
	std::string settingsFile;
	std::string envFile;
	std::string rootDirectory;
	std::string outputDirectory;
	std::string bundleDirectory;
	std::string signingIdentity;

	uint maxJobs = 0;
	bool benchmark = true;
	bool showCommands = false;
	bool dumpAssembly = false;
	bool generateCompileCommands = false;
};
}

#endif // CHALET_GLOBAL_SETTINGS_STATE_HPP
