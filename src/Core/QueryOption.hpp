/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class QueryOption : ushort
{
	None,
	QueryNames,
	Version,
	//
	Commands,
	Options,
	//
	Configurations,
	AllToolchains,
	UserToolchains,
	ToolchainPresets,
	Architectures,
	AllBuildTargets,
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
