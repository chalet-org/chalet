/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_INTERMEDIATE_SETTINGS_STATE_HPP
#define CHALET_INTERMEDIATE_SETTINGS_STATE_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
struct IntermediateSettingsState
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
	std::string externalDirectory;
	std::string distributionDirectory;
	std::string signingIdentity;
	std::string runTarget;

	uint maxJobs = 0;
	bool benchmark = false;
	bool launchProfiler = false;
	bool keepGoing = false;
	bool showCommands = false;
	bool dumpAssembly = false;
	bool generateCompileCommands = false;
};
}

#endif // CHALET_INTERMEDIATE_SETTINGS_STATE_HPP
