/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ARGUMENT_IDENTIFIER_HPP
#define CHALET_ARGUMENT_IDENTIFIER_HPP

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
	RunTargetName,
	RunTargetArguments,
	SaveSchema,
	Quieter,
	LocalSettings,
	GlobalSettings,
	DumpAssembly,
	GenerateCompileCommands,
	ShowCommands,
	Benchmark,
	LaunchProfiler,
	KeepGoing,
	SaveUserToolchainGlobally,
	SigningIdentity,
	OsTargetName,
	OsTargetVersion,
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

#endif // CHALET_ARGUMENT_IDENTIFIER_HPP
