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
	const std::string kKeyBuild = "build";
	const std::string kKeyToolchains = "toolchains";
	const std::string kKeyTools = "tools";
	const std::string kKeyAppleSdks = "appleSdks";

	const std::string kKeyDumpAssembly = "dumpAssembly";
	const std::string kKeyMaxJobs = "maxJobs";
	const std::string kKeyShowCommands = "showCommands";
	const std::string kKeyLastToolchain = "toolchain";
	const std::string kKeyMacosSigningIdentity = "macosSigningIdentity";
};
}

#endif // CHALET_ISETTINGS_JSON_PARSER_HPP
