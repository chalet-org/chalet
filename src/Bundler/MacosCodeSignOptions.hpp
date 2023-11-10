/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

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
