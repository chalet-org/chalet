/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_QUERY_OPTION_HPP
#define CHALET_QUERY_OPTION_HPP

namespace chalet
{
enum class QueryOption : ushort
{
	None,
	QueryNames,
	//
	Commands,
	Configurations,
	AllToolchains,
	UserToolchains,
	ToolchainPresets,
	Architectures,
	AllRunTargets,
	//
	Toolchain,
	Configuration,
	Architecture,
	RunTarget,
	//
	ThemeNames,
	//
	ChaletJsonState,
	SettingsJsonState,
	//
	ChaletSchema,
	SettingsSchema,
};
}

#endif // CHALET_QUERY_OPTION_HPP
