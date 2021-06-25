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

	std::string macosSigningIdentity;
	std::string toolchainPreference;
	uint maxJobs = 0;
	bool showCommands = false;
	bool dumpAssembly = false;
};
}

#endif // CHALET_GLOBAL_SETTINGS_STATE_HPP
