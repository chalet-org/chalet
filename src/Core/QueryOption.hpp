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
	Version,
	//
	Commands,
	Configurations,
	AllToolchains,
	UserToolchains,
	ToolchainPresets,
	Architectures,
	AllRunTargets,
	BuildStrategies,
	BuildPathStyles,
	//
	Toolchain,
	Configuration,
	Architecture,
	RunTarget,
	BuildStrategy,
	BuildPathStyle,
	//
	ExportKinds,
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
