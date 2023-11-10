/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class ArgumentIdentifier : ushort
{
	None,
	//
	SubCommand,
	Help,
	Version,
	//
	InputFile,
	SettingsFile,
	File,
	RootDirectory,
	ExternalDirectory,
	OutputDirectory,
	DistributionDirectory,
	// ProjectGen
	EnvFile,
	BuildConfiguration,
	Toolchain,
	TargetArchitecture,
	BuildStrategy,
	BuildPathStyle,
	MaxJobs,
	BuildTargetName,
	RunTargetArguments,
	SaveSchema,
	Quieter,
	LocalSettings,
	GlobalSettings,
	DumpAssembly,
	GenerateCompileCommands,
	OnlyRequired,
	ShowCommands,
	Benchmark,
	LaunchProfiler,
	KeepGoing,
	SaveUserToolchainGlobally,
	SigningIdentity,
	OsTargetName,
	OsTargetVersion,
	ValidateSchemaFile,
	ValidateFilesRemainingArgs,
	//
	// Init
	InitPath,
	InitTemplate,
	//
	// Export
	ExportKind,
	//
	// Other
	RouteString,
	SettingsKey,
	SettingsValue,
	QueryType,
	QueryDataRemainingArgs,
	SettingsKeysRemainingArgs,
};
}
