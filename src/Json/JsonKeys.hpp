/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_JSON_KEYS_HPP
#define CHALET_JSON_KEYS_HPP

#define CH_STR(x) static constexpr const char x[]

namespace chalet
{
namespace Keys
{
//
// chalet.json
CH_STR(Workspace) = "workspace";
CH_STR(Version) = "version";
CH_STR(Abstracts) = "abstracts";
CH_STR(AbstractsAll) = "abstracts:*";
CH_STR(Targets) = "targets";
CH_STR(SettingsCxx) = "settings:Cxx";
CH_STR(Distribution) = "distribution";
CH_STR(Configurations) = "configurations";
CH_STR(DefaultConfigurations) = "defaultConfigurations";
CH_STR(ExternalDependencies) = "externalDependencies";

//
CH_STR(Kind) = "kind";
CH_STR(RunExecutable) = "runExecutable";

//
// .chaletrc
CH_STR(Options) = "options";
CH_STR(OptionsToolchain) = "toolchain";
CH_STR(OptionsArchitecture) = "architecture";
CH_STR(OptionsBuildConfiguration) = "configuration";
CH_STR(OptionsDumpAssembly) = "dumpAssembly";
CH_STR(OptionsGenerateCompileCommands) = "generateCompileCommands";
CH_STR(OptionsMaxJobs) = "maxJobs";
CH_STR(OptionsShowCommands) = "showCommands";
CH_STR(OptionsBenchmark) = "benchmark";
CH_STR(OptionsLaunchProfiler) = "launchProfiler";
CH_STR(OptionsKeepGoing) = "keepGoing";
CH_STR(OptionsSigningIdentity) = "signingIdentity";
CH_STR(OptionsInputFile) = "inputFile";
CH_STR(OptionsEnvFile) = "envFile";
CH_STR(OptionsRootDirectory) = "rootDir";
CH_STR(OptionsOutputDirectory) = "outputDir";
CH_STR(OptionsExternalDirectory) = "externalDir";
CH_STR(OptionsDistributionDirectory) = "distributionDir";
CH_STR(OptionsRunTarget) = "runTarget";
CH_STR(OptionsRunArguments) = "runArguments";
//
CH_STR(Toolchains) = "toolchains";
CH_STR(ToolchainVersion) = "version";
CH_STR(ToolchainStrategy) = "strategy";
CH_STR(ToolchainBuildPathStyle) = "buildPathStyle";
CH_STR(ToolchainArchiver) = "archiver";
CH_STR(ToolchainCompilerCpp) = "compilerCpp";
CH_STR(ToolchainCompilerC) = "compilerC";
CH_STR(ToolchainCompilerWindowsResource) = "compilerWindowsResource";
CH_STR(ToolchainLinker) = "linker";
CH_STR(ToolchainProfiler) = "profiler";
CH_STR(ToolchainDisassembler) = "disassembler";
CH_STR(ToolchainCMake) = "cmake";
CH_STR(ToolchainMake) = "make";
CH_STR(ToolchainNinja) = "ninja";
//
CH_STR(Tools) = "tools";
CH_STR(ToolsBash) = "bash";
CH_STR(ToolsCodesign) = "codesign";
CH_STR(ToolsCommandPrompt) = "command_prompt";
CH_STR(ToolsGit) = "git";
CH_STR(ToolsHdiutil) = "hdiutil";
CH_STR(ToolsInstallNameTool) = "install_name_tool";
CH_STR(ToolsInstruments) = "instruments";
CH_STR(ToolsLdd) = "ldd";
CH_STR(ToolsMakeNsis) = "makensis";
CH_STR(ToolsOsascript) = "osascript";
CH_STR(ToolsOtool) = "otool";
CH_STR(ToolsPlutil) = "plutil";
CH_STR(ToolsPowershell) = "powershell";
CH_STR(ToolsSample) = "sample";
CH_STR(ToolsSips) = "sips";
CH_STR(ToolsTiffutil) = "tiffutil";
CH_STR(ToolsXcodebuild) = "xcodebuild";
// CH_STR(ToolsXcodegen) = "xcodegen";
CH_STR(ToolsXcrun) = "xcrun";
CH_STR(ToolsZip) = "zip";
//
CH_STR(AppleSdks) = "appleSdks";
CH_STR(Theme) = "theme";
}

namespace SKeys
{
CH_STR(Definitions) = "definitions";
CH_STR(Items) = "items";
CH_STR(Properties) = "properties";
CH_STR(AdditionalProperties) = "additionalProperties";
CH_STR(Pattern) = "pattern";
CH_STR(PatternProperties) = "patternProperties";
CH_STR(Description) = "description";
CH_STR(Default) = "default";
CH_STR(Enum) = "enum";
CH_STR(Examples) = "examples";
CH_STR(AnyOf) = "anyOf";
CH_STR(AllOf) = "allOf";
CH_STR(OneOf) = "oneOf";
CH_STR(Then) = "then";
CH_STR(Else) = "else";
}

namespace CacheKeys
{
CH_STR(DataVersion) = "v";
CH_STR(DataArch) = "a";
CH_STR(DataExternalRebuild) = "e";
CH_STR(Hashes) = "h";
CH_STR(HashBuild) = "b";
CH_STR(HashTheme) = "t";
CH_STR(HashVersionDebug) = "vd";
CH_STR(HashVersionRelease) = "vr";
CH_STR(HashMetadata) = "wm";
CH_STR(HashExtra) = "e";
CH_STR(LastChaletJsonWriteTime) = "c";
CH_STR(Builds) = "d";
CH_STR(BuildLastBuilt) = "l";
CH_STR(BuildLastBuildStrategy) = "s";
CH_STR(BuildFiles) = "f";
CH_STR(DataCache) = "x";
}

namespace MSVCKeys
{
// Version 1.1
CH_STR(Version) = "Version";
CH_STR(Data) = "Data";
CH_STR(ProvidedModule) = "ProvidedModule";
CH_STR(ImportedModules) = "ImportedModules";
CH_STR(ImportedHeaderUnits) = "ImportedHeaderUnits";
}
}

#undef CH_STR

#endif // CHALET_JSON_KEYS_HPP
