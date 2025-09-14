/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class ArgumentIdentifier : u16
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
	CompilerCache,
	SaveUserToolchainGlobally,
	SigningIdentity,
	ProfilerConfig,
	OsTargetName,
	OsTargetVersion,
	ValidateSchemaFile,
	ValidateFilesRemainingArgs,
	//
	// Clean
	CleanAll,
	//
	// Init
	InitPath,
	InitTemplate,
	//
	// Export
	ExportKind,
	ExportOpen,
	ExportBuildConfigurations,
	ExportArchitectures,
	//
	// Other
	RouteString,
	SettingsKey,
	SettingsValueRemainingArgs,
	QueryType,
	QueryDataRemainingArgs,
	SettingsKeysRemainingArgs,
	ConvertFormat,
};
}
