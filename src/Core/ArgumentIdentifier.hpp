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
	MaxJobs,
	RunProjectName,
	RunProjectArguments,
	SaveSchema,
	Quieter,
	LocalSettings,
	GlobalSettings,
	DumpAssembly,
	GenerateCompileCommands,
	ShowCommands,
	Benchmark,
	//
	// Other
	RouteString,
	InitPath,
	SettingsKey,
	SettingsValue,
};
}

#endif // CHALET_ARGUMENT_IDENTIFIER_HPP
