/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MACOS_CODE_SIGN_OPTIONS_HPP
#define CHALET_MACOS_CODE_SIGN_OPTIONS_HPP

namespace chalet
{
struct MacosCodeSignOptions
{
	std::string entitlementsFile;

	bool timestamp = true;
	bool hardenedRuntime = true;
	bool strict = true;
	bool force = true;
};
}

#endif // CHALET_MACOS_CODE_SIGN_OPTIONS_HPP
