/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
namespace Keys
{
//
// chalet.json
CHALET_CONSTANT(WorkspaceName) = "name";
CHALET_CONSTANT(WorkspaceVersion) = "version";
CHALET_CONSTANT(PlatformRequires) = "platformRequires";
CHALET_CONSTANT(Variables) = "variables";
CHALET_CONSTANT(Abstracts) = "abstracts";
CHALET_CONSTANT(AbstractsAll) = "abstracts:*";
CHALET_CONSTANT(Targets) = "targets";
CHALET_CONSTANT(SettingsCxx) = "settings:Cxx";
CHALET_CONSTANT(Distribution) = "distribution";
CHALET_CONSTANT(Configurations) = "configurations";
CHALET_CONSTANT(AllowedArchitectures) = "allowedArchitectures";
CHALET_CONSTANT(DefaultConfigurations) = "defaultConfigurations";
CHALET_CONSTANT(ExternalDependencies) = "externalDependencies";
CHALET_CONSTANT(SearchPaths) = "searchPaths";

//
CHALET_CONSTANT(Kind) = "kind";
CHALET_CONSTANT(RunExecutable) = "runExecutable";

//
#if defined(CHALET_WIN32)
CHALET_CONSTANT(ReqWindowsMSYS2) = "windows.msys2";
#elif defined(CHALET_MACOS)
CHALET_CONSTANT(ReqMacOSMacPorts) = "macos.macports";
CHALET_CONSTANT(ReqMacOSHomebrew) = "macos.homebrew";
#else
CHALET_CONSTANT(ReqUbuntuSystem) = "ubuntu.system";
CHALET_CONSTANT(ReqDebianSystem) = "debian.system";
CHALET_CONSTANT(ReqArchLinuxSystem) = "archlinux.system";
CHALET_CONSTANT(ReqManjaroSystem) = "manjaro.system";
CHALET_CONSTANT(ReqFedoraSystem) = "fedora.system";
CHALET_CONSTANT(ReqRedHatSystem) = "redhat.system";
#endif

//
// .chaletrc
CHALET_CONSTANT(Options) = "options";
CHALET_CONSTANT(OptionsToolchain) = "toolchain";
CHALET_CONSTANT(OptionsArchitecture) = "architecture";
CHALET_CONSTANT(OptionsBuildConfiguration) = "configuration";
CHALET_CONSTANT(OptionsDumpAssembly) = "dumpAssembly";
CHALET_CONSTANT(OptionsGenerateCompileCommands) = "generateCompileCommands";
CHALET_CONSTANT(OptionsOnlyRequired) = "onlyRequired";
CHALET_CONSTANT(OptionsMaxJobs) = "maxJobs";
CHALET_CONSTANT(OptionsShowCommands) = "showCommands";
CHALET_CONSTANT(OptionsBenchmark) = "benchmark";
CHALET_CONSTANT(OptionsLaunchProfiler) = "launchProfiler";
CHALET_CONSTANT(OptionsKeepGoing) = "keepGoing";
CHALET_CONSTANT(OptionsSigningIdentity) = "signingIdentity";
CHALET_CONSTANT(OptionsOsTargetName) = "osTargetName";
CHALET_CONSTANT(OptionsOsTargetVersion) = "osTargetVersion";
CHALET_CONSTANT(OptionsInputFile) = "inputFile";
CHALET_CONSTANT(OptionsEnvFile) = "envFile";
CHALET_CONSTANT(OptionsRootDirectory) = "rootDir";
CHALET_CONSTANT(OptionsOutputDirectory) = "outputDir";
CHALET_CONSTANT(OptionsExternalDirectory) = "externalDir";
CHALET_CONSTANT(OptionsDistributionDirectory) = "distributionDir";
CHALET_CONSTANT(OptionsLastTarget) = "lastTarget";
CHALET_CONSTANT(OptionsRunArguments) = "runArguments";
//
CHALET_CONSTANT(Toolchains) = "toolchains";
CHALET_CONSTANT(ToolchainVersion) = "version";
CHALET_CONSTANT(ToolchainBuildStrategy) = "strategy";
CHALET_CONSTANT(ToolchainBuildPathStyle) = "buildPathStyle";
CHALET_CONSTANT(ToolchainArchiver) = "archiver";
CHALET_CONSTANT(ToolchainCompilerCpp) = "compilerCpp";
CHALET_CONSTANT(ToolchainCompilerC) = "compilerC";
CHALET_CONSTANT(ToolchainCompilerWindowsResource) = "compilerWindowsResource";
CHALET_CONSTANT(ToolchainLinker) = "linker";
CHALET_CONSTANT(ToolchainProfiler) = "profiler";
CHALET_CONSTANT(ToolchainDisassembler) = "disassembler";
CHALET_CONSTANT(ToolchainCMake) = "cmake";
CHALET_CONSTANT(ToolchainMake) = "make";
CHALET_CONSTANT(ToolchainNinja) = "ninja";
//
CHALET_CONSTANT(Tools) = "tools";
CHALET_CONSTANT(ToolsBash) = "bash";
CHALET_CONSTANT(ToolsCodesign) = "codesign";
CHALET_CONSTANT(ToolsCommandPrompt) = "command_prompt";
CHALET_CONSTANT(ToolsGit) = "git";
CHALET_CONSTANT(ToolsHdiutil) = "hdiutil";
CHALET_CONSTANT(ToolsInstallNameTool) = "install_name_tool";
CHALET_CONSTANT(ToolsInstruments) = "instruments";
CHALET_CONSTANT(ToolsLdd) = "ldd";
CHALET_CONSTANT(ToolsOsascript) = "osascript";
CHALET_CONSTANT(ToolsOtool) = "otool";
CHALET_CONSTANT(ToolsPlutil) = "plutil";
CHALET_CONSTANT(ToolsPowershell) = "powershell";
CHALET_CONSTANT(ToolsSample) = "sample";
CHALET_CONSTANT(ToolsSips) = "sips";
CHALET_CONSTANT(ToolsTar) = "tar";
CHALET_CONSTANT(ToolsTiffutil) = "tiffutil";
CHALET_CONSTANT(ToolsXcodebuild) = "xcodebuild";
CHALET_CONSTANT(ToolsXcrun) = "xcrun";
CHALET_CONSTANT(ToolsZip) = "zip";
//
CHALET_CONSTANT(AppleSdks) = "appleSdks";
CHALET_CONSTANT(Theme) = "theme";
CHALET_CONSTANT(LastUpdateCheck) = "lastUpdateCheck";
}

namespace SKeys
{
// Note: dont' use if/else - less supported (looking at you, IntelliJ/CLion)
//
CHALET_CONSTANT(Definitions) = "definitions";
CHALET_CONSTANT(Items) = "items";
CHALET_CONSTANT(UniqueItems) = "uniqueItems";
CHALET_CONSTANT(Properties) = "properties";
CHALET_CONSTANT(AdditionalProperties) = "additionalProperties";
CHALET_CONSTANT(Pattern) = "pattern";
CHALET_CONSTANT(PatternProperties) = "patternProperties";
CHALET_CONSTANT(Description) = "description";
CHALET_CONSTANT(Default) = "default";
CHALET_CONSTANT(Enum) = "enum";
CHALET_CONSTANT(Const) = "const";
CHALET_CONSTANT(Examples) = "examples";
CHALET_CONSTANT(AnyOf) = "anyOf";
CHALET_CONSTANT(AllOf) = "allOf";
CHALET_CONSTANT(OneOf) = "oneOf";
}

namespace CacheKeys
{
CHALET_CONSTANT(DataVersion) = "v";
CHALET_CONSTANT(DataArch) = "a";
CHALET_CONSTANT(DataExternalRebuild) = "e";
CHALET_CONSTANT(Hashes) = "h";
CHALET_CONSTANT(HashBuild) = "b";
CHALET_CONSTANT(HashTheme) = "t";
CHALET_CONSTANT(HashVersionDebug) = "vd";
CHALET_CONSTANT(HashVersionRelease) = "vr";
CHALET_CONSTANT(HashMetadata) = "wm";
CHALET_CONSTANT(HashExtra) = "e";
CHALET_CONSTANT(LastChaletJsonWriteTime) = "c";
CHALET_CONSTANT(Builds) = "d";
CHALET_CONSTANT(BuildLastBuilt) = "l";
CHALET_CONSTANT(BuildLastBuildStrategy) = "s";
CHALET_CONSTANT(BuildFiles) = "f";
CHALET_CONSTANT(DataCache) = "x";
}

namespace MSVCKeys
{
// Version 1.1
CHALET_CONSTANT(Version) = "Version";
CHALET_CONSTANT(Data) = "Data";
CHALET_CONSTANT(ProvidedModule) = "ProvidedModule";
CHALET_CONSTANT(ImportedModules) = "ImportedModules";
CHALET_CONSTANT(ImportedHeaderUnits) = "ImportedHeaderUnits";
CHALET_CONSTANT(Includes) = "Includes";
CHALET_CONSTANT(Header) = "Header";
CHALET_CONSTANT(BMI) = "BMI";
}
}
