/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_GLOBAL_CONFIG_STATE_HPP
#define CHALET_GLOBAL_CONFIG_STATE_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
struct GlobalConfigState
{
	Json toolchains;

	std::string macosCertSigningRequest;
	std::string toolchainPreference;
	ushort maxJobs = 0;
	bool showCommands = false;
	bool dumpAssembly = false;
};
}

#endif // CHALET_GLOBAL_CONFIG_STATE_HPP
