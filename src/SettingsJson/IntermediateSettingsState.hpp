/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

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
	std::string profilerConfig;
	std::string osTargetName;
	std::string osTargetVersion;
	std::string lastTarget;

	u32 maxJobs = 0;
	bool benchmark = false;
	bool launchProfiler = false;
	bool keepGoing = false;
	bool compilerCache = false;
	bool showCommands = false;
	bool dumpAssembly = false;
	bool generateCompileCommands = false;
	bool onlyRequired = false;
};
}
