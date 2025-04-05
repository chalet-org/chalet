/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CodeLanguage.hpp"
#include "Compile/PositionIndependentCodeType.hpp"
#include "State/ProjectWarningPresets.hpp"
#include "State/SourceKind.hpp"
#include "State/SourceType.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/WindowsEntryPoint.hpp"
#include "State/WindowsSubSystem.hpp"
#include "Utility/GlobMatch.hpp"

namespace chalet
{
struct TargetMetadata;
class BuildState;

struct SourceTarget final : public IBuildTarget
{
	explicit SourceTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;
	virtual const std::string& getHash() const final;

	bool hasMetadata() const noexcept;
	const TargetMetadata& metadata() const noexcept;
	void setMetadata(Ref<TargetMetadata>&& inValue);

	const StringList& defines() const noexcept;
	void addDefines(StringList&& inList);
	void addDefine(std::string&& inValue);

	const StringList& links() const noexcept;
	void addLinks(StringList&& inList);
	void addLink(std::string&& inValue);
	bool resolveLinksFromProject(const std::vector<BuildTarget>& inTargets, const std::string& inInputFile);

	const StringList& projectStaticLinks() const noexcept;
	const StringList& projectSharedLinks() const noexcept;

	const StringList& staticLinks() const noexcept;
	void addStaticLinks(StringList&& inList);
	void addStaticLink(std::string&& inValue);

	const StringList& libDirs() const noexcept;
	void addLibDirs(StringList&& inList);
	void addLibDir(std::string&& inValue);

	const StringList& includeDirs() const noexcept;
	void addIncludeDirs(StringList&& inList);
	void addIncludeDir(std::string&& inValue);

	const StringList& warnings() const noexcept;
	void addWarnings(StringList&& inList);
	void addWarning(std::string&& inValue);
	void setWarningPreset(std::string&& inValue);
	ProjectWarningPresets warningsPreset() const noexcept;

	const StringList& compileOptions() const noexcept;
	void addCompileOptions(StringList&& inList);
	void addCompileOption(std::string&& inValue);

	const StringList& linkerOptions() const noexcept;
	void addLinkerOptions(StringList&& inList);
	void addLinkerOption(std::string&& inValue);

	const StringList& appleFrameworkPaths() const noexcept;
	void addAppleFrameworkPaths(StringList&& inList);
	void addAppleFrameworkPath(std::string&& inValue);

	const StringList& appleFrameworks() const noexcept;
	void addAppleFrameworks(StringList&& inList);
	void addAppleFramework(std::string&& inValue);

	const StringList& copyFilesOnRun() const noexcept;
	void addCopyFilesOnRun(StringList&& inList);
	void addCopyFileOnRun(std::string&& inValue);

	const StringList& importPackages() const noexcept;
	void addImportPackages(StringList&& inList);
	void addImportPackage(std::string&& inValue);

	const StringList& ccacheOptions() const noexcept;
	void addCcacheOptions(StringList&& inList);
	void addCcacheOption(std::string&& inValue);

	//
	void parseOutputFilename() noexcept;

	const std::string& outputFile() const noexcept;
	std::string getOutputFileWithoutPrefix() const noexcept;

	CodeLanguage language() const noexcept;
	void setLanguage(const std::string& inValue) noexcept;
	SourceType getDefaultSourceType() const;

	const std::string& cStandard() const noexcept;
	void setCStandard(std::string&& inValue) noexcept;

	const std::string& cppStandard() const noexcept;
	void setCppStandard(std::string&& inValue) noexcept;

	const StringList& files() const noexcept;
	void addFiles(StringList&& inList);
	void addFile(std::string&& inValue);
	StringList getHeaderFiles() const;

	const StringList& fileExcludes() const noexcept;
	void addFileExcludes(StringList&& inList);
	void addFileExclude(std::string&& inValue);

	const StringList& configureFiles() const noexcept;
	void addConfigureFiles(StringList&& inList);
	void addConfigureFile(std::string&& inValue);

	const std::string& precompiledHeader() const noexcept;
	void setPrecompiledHeader(std::string&& inValue) noexcept;
	bool usesPrecompiledHeader() const noexcept;

	const std::string& inputCharset() const noexcept;
	void setInputCharset(std::string&& inValue) noexcept;

	const std::string& executionCharset() const noexcept;
	void setExecutionCharset(std::string&& inValue) noexcept;

	const std::string& windowsApplicationManifest() const noexcept;
	void setWindowsApplicationManifest(std::string&& inValue) noexcept;

	bool windowsApplicationManifestGenerationEnabled() const noexcept;
	void setWindowsApplicationManifestGenerationEnabled(const bool inValue) noexcept;

	const std::string& windowsApplicationIcon() const noexcept;
	void setWindowsApplicationIcon(std::string&& inValue) noexcept;

	const std::string& buildSuffix() const noexcept;
	void setBuildSuffix(std::string&& inValue) noexcept;

	const std::string& workingDirectory() const noexcept;
	void setWorkingDirectory(std::string&& inValue) noexcept;

	//
	SourceKind kind() const noexcept;
	void setKind(const SourceKind inValue) noexcept;
	void setKind(const std::string& inValue);
	bool isExecutable() const noexcept;
	bool isSharedLibrary() const noexcept;
	bool isStaticLibrary() const noexcept;

	WindowsSubSystem windowsSubSystem() const noexcept;
	void setWindowsSubSystem(const WindowsSubSystem inValue) noexcept;
	void setWindowsSubSystem(const std::string& inValue);

	WindowsEntryPoint windowsEntryPoint() const noexcept;
	void setWindowsEntryPoint(const WindowsEntryPoint inValue) noexcept;
	void setWindowsEntryPoint(const std::string& inValue);

	bool positionIndependentCode() const noexcept;
	bool positionIndependentExecutable() const noexcept;
	void setPicType(const bool inValue) noexcept;
	void setPicType(const std::string& inValue);

	bool threads() const noexcept;
	void setThreads(const bool inValue) noexcept;

	bool treatWarningsAsErrors() const noexcept;
	void setTreatWarningsAsErrors(const bool inValue) noexcept;

	bool cppFilesystem() const noexcept;
	void setCppFilesystem(const bool inValue) noexcept;

	bool cppModules() const noexcept;
	void setCppModules(const bool inValue) noexcept;

	bool cppCoroutines() const noexcept;
	void setCppCoroutines(const bool inValue) noexcept;

	bool cppConcepts() const noexcept;
	void setCppConcepts(const bool inValue) noexcept;

	bool objectiveCxx() const noexcept;

	bool runtimeTypeInformation() const noexcept;
	void setRuntimeTypeInformation(const bool inValue) noexcept;

	bool exceptions() const noexcept;
	void setExceptions(const bool inValue) noexcept;

	bool fastMath() const noexcept;
	void setFastMath(const bool inValue) noexcept;

	bool staticRuntimeLibrary() const noexcept;
	void setStaticRuntimeLibrary(const bool inValue) noexcept;

	bool unityBuild() const noexcept;
	void setUnityBuild(const bool inValue) noexcept;

	void setMinGWUnixSharedLibraryNamingConvention(const bool inValue) noexcept;

	bool windowsOutputDef() const noexcept;
	void setWindowsOutputDef(const bool inValue) noexcept;

	bool justMyCodeDebugging() const noexcept;
	void setJustMyCodeDebugging(const bool inValue) noexcept;

	StringList getResolvedRunDependenciesList() const;

	bool generateUnityBuildFile(std::string& outSourceFile) const;

private:
	bool removeExcludedFiles();
	bool determinePicType();
	bool initializeUnityBuild();

	std::string getUnityBuildFile() const;
	std::string getPrecompiledHeaderResolvedToRoot() const;

	SourceKind parseProjectKind(const std::string& inValue);
	WindowsSubSystem parseWindowsSubSystem(const std::string& inValue);
	WindowsEntryPoint parseWindowsEntryPoint(const std::string& inValue);
	ProjectWarningPresets parseWarnings(const std::string& inValue);

	Ref<TargetMetadata> m_metadata;

	StringList m_defines;
	StringList m_links;
	StringList m_projectStaticLinks;
	StringList m_projectSharedLinks;
	StringList m_staticLinks;
	StringList m_libDirs;
	StringList m_includeDirs;
	StringList m_warnings;
	StringList m_compileOptions;
	StringList m_linkerOptions;
	StringList m_appleFrameworkPaths;
	StringList m_appleFrameworks;
	StringList m_copyFilesOnRun;
	StringList m_files;
	StringList m_headers;
	StringList m_fileExcludes;
	StringList m_configureFiles;
	StringList m_importPackages;
	StringList m_ccacheOptions;

	std::string m_warningsPresetString;
	std::string m_outputFile;
	std::string m_cStandard;
	std::string m_cppStandard;
	std::string m_precompiledHeader;
	std::string m_inputCharset;
	std::string m_executionCharset;
	std::string m_windowsApplicationManifest;
	std::string m_windowsApplicationIcon;
	std::string m_buildSuffix;
	std::string m_unityBuildContents;
	std::string m_workingDirectory;

	SourceKind m_kind = SourceKind::None;
	CodeLanguage m_language = CodeLanguage::None;
	ProjectWarningPresets m_warningsPreset = ProjectWarningPresets::None;
	WindowsSubSystem m_windowsSubSystem = WindowsSubSystem::Console;
	WindowsEntryPoint m_windowsEntryPoint = WindowsEntryPoint::Main;
	PositionIndependentCodeType m_picType = PositionIndependentCodeType::None;

	bool m_threads = true;
	bool m_cppFilesystem = false;
	bool m_cppModules = false;
	bool m_cppCoroutines = false;
	bool m_cppConcepts = false;
	bool m_runtimeTypeInformation = true;
	bool m_exceptions = true;
	bool m_fastMath = false;
	bool m_staticRuntimeLibrary = false;
	bool m_treatWarningsAsErrors = false;
	bool m_posixThreads = true;
	bool m_invalidWarningPreset = false;
	bool m_unityBuild = false;
	bool m_windowsApplicationManifestGenerationEnabled = true;
	bool m_mingwUnixSharedLibraryNamingConvention = true;
	bool m_setWindowsPrefixOutputFilename = false;
	bool m_windowsOutputDef = false;
	bool m_justMyCodeDebugging = true;
};
}
