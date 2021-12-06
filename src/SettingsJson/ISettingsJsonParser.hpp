/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ISETTINGS_JSON_PARSER_HPP
#define CHALET_ISETTINGS_JSON_PARSER_HPP

namespace chalet
{
struct ISettingsJsonParser
{
	virtual ~ISettingsJsonParser() = default;

protected:
	const std::string kKeyOptions = "options";
	const std::string kKeyToolchains = "toolchains";
	const std::string kKeyTools = "tools";
	const std::string kKeyAppleSdks = "appleSdks";
	const std::string kKeyTheme = "theme";

	const std::string kKeyDumpAssembly = "dumpAssembly";
	const std::string kKeyGenerateCompileCommands = "generateCompileCommands";
	const std::string kKeyMaxJobs = "maxJobs";
	const std::string kKeyShowCommands = "showCommands";
	const std::string kKeyBenchmark = "benchmark";
	const std::string kKeyLaunchProfiler = "launchProfiler";
	const std::string kKeyLastBuildConfiguration = "configuration";
	const std::string kKeyLastToolchain = "toolchain";
	const std::string kKeyLastArchitecture = "architecture";
	const std::string kKeySigningIdentity = "signingIdentity";

	const std::string kKeyInputFile = "inputFile";
	const std::string kKeyEnvFile = "envFile";
	const std::string kKeyRootDirectory = "rootDir";
	const std::string kKeyOutputDirectory = "outputDir";
	const std::string kKeyExternalDirectory = "externalDir";
	const std::string kKeyDistributionDirectory = "distributionDir";
};
}

#endif // CHALET_ISETTINGS_JSON_PARSER_HPP
