/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ICONFIG_JSON_PARSER_HPP
#define CHALET_ICONFIG_JSON_PARSER_HPP

namespace chalet
{
struct IConfigJsonParser
{
	virtual ~IConfigJsonParser() = default;

protected:
	const std::string kKeySettings = "settings";
	const std::string kKeyToolchains = "toolchains";
	const std::string kKeyAncillaryTools = "ancillaryTools";
	const std::string kKeyApplePlatformSdks = "applePlatformSdks";

	const std::string kKeyDumpAssembly = "dumpAssembly";
	const std::string kKeyMaxJobs = "maxJobs";
	const std::string kKeyShowCommands = "showCommands";
	const std::string kKeyLastToolchain = "toolchain";
	const std::string kKeyMacosSigningIdentity = "macosSigningIdentity";
};
}

#endif // CHALET_ICONFIG_JSON_PARSER_HPP
