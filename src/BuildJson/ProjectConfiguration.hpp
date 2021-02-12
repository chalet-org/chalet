/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROJECT_CONFIGURATION_HPP
#define CHALET_PROJECT_CONFIGURATION_HPP

#include "BuildJson/BuildEnvironment.hpp"
#include "BuildJson/ProjectKind.hpp"
#include "Compile/CodeLanguage.hpp"
#include "State/CommandLineInputs.hpp"

namespace chalet
{
struct ProjectConfiguration
{
	explicit ProjectConfiguration(const std::string& inBuildConfig, const BuildEnvironment& inEnvironment);

	bool isExecutable() const noexcept;

	const StringList& fileExtensions() const noexcept;
	void addFileExtensions(StringList& inList);
	void addFileExtension(std::string& inValue);

	const StringList& defines() const noexcept;
	void addDefines(StringList& inList);
	void addDefine(std::string& inValue);

	const StringList& links() const noexcept;
	void addLinks(StringList& inList);
	void addLink(std::string& inValue);
	void resolveLinksFromProject(const std::string& inProjectName, const bool inStaticLib);

	const StringList& staticLinks() const noexcept;
	void addStaticLinks(StringList& inList);
	void addStaticLink(std::string& inValue);

	const StringList& libDirs() const noexcept;
	void addLibDirs(StringList& inList);
	void addLibDir(std::string& inValue);

	const StringList& includeDirs() const noexcept;
	void addIncludeDirs(StringList& inList);
	void addIncludeDir(std::string& inValue);

	const StringList& runDependencies() const noexcept;
	void addRunDependencies(StringList& inList);
	void addRunDependency(std::string& inValue);

	const StringList& warnings() const noexcept;
	void addWarnings(StringList& inList);
	void addWarning(std::string& inValue);
	void setWarningPreset(const std::string& inValue);

	bool includeInBuild() const noexcept;
	void setIncludeInBuild(const bool inValue);

	const StringList& compileOptions() const noexcept;
	void addCompileOptions(StringList& inList);
	void addCompileOption(std::string& inValue);

	const StringList& linkerOptions() const noexcept;
	void addLinkerOptions(StringList& inList);
	void addLinkerOption(std::string& inValue);

	const StringList& macosFrameworkPaths() const noexcept;
	void addMacosFrameworkPaths(StringList& inList);
	void addMacosFrameworkPath(std::string& inValue);

	const StringList& macosFrameworks() const noexcept;
	void addMacosFrameworks(StringList& inList);
	void addMacosFramework(std::string& inValue);

	//
	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;
	void parseOutputFilename() noexcept;

	const std::string& outputFile() const noexcept;

	CodeLanguage language() const noexcept;
	void setLanguage(const std::string& inValue) noexcept;

	const std::string& cStandard() const noexcept;
	void setCStandard(const std::string& inValue) noexcept;

	const std::string& cppStandard() const noexcept;
	void setCppStandard(const std::string& inValue) noexcept;

	const StringList& files() const noexcept;
	void addFiles(StringList& inList);
	void addFile(std::string& inValue);

	const StringList& locations() const noexcept;
	void addLocations(StringList& inList);
	void addLocation(std::string& inValue);

	const StringList& locationExcludes() const noexcept;
	void addLocationExcludes(StringList& inList);
	void addLocationExclude(std::string& inValue);

	const StringList& preBuildScripts() const noexcept;
	void addPreBuildScripts(StringList& inList);
	void addPreBuildScript(std::string& inValue);

	const StringList& postBuildScripts() const noexcept;
	void addPostBuildScripts(StringList& inList);
	void addPostBuildScript(std::string& inValue);

	bool alwaysRunPostBuildScripts() const noexcept;
	void setAlwaysRunPostBuildScripts(const bool inValue) noexcept;

	const std::string& pch() const noexcept;
	void setPch(const std::string& inValue) noexcept;
	bool usesPch() const noexcept;

	const std::string& runArguments() const noexcept;
	void setRunArguments(const std::string& inValue) noexcept;

	const std::string& linkerScript() const noexcept;
	void setLinkerScript(const std::string& inValue) noexcept;

	//
	ProjectKind kind() const noexcept;
	void setKind(const ProjectKind inValue) noexcept;
	void setKind(const std::string& inValue);

	//
	bool cmake() const noexcept;
	void setCmake(const bool inValue) noexcept;

	bool cmakeRecheck() const noexcept;
	void setCmakeRecheck(const bool inValue) noexcept;

	const StringList& cmakeDefines() const noexcept;
	void addCmakeDefines(StringList& inList);
	void addCmakeDefine(std::string& inValue);

	bool dumpAssembly() const noexcept;
	void setDumpAssembly(const bool inValue) noexcept;

	bool objectiveCxx() const noexcept;
	void setObjectiveCxx(const bool inValue) noexcept;

	bool rtti() const noexcept;
	void setRtti(const bool inValue) noexcept;

	bool runProject() const noexcept;
	void setRunProject(const bool inValue) noexcept;

	bool staticLinking() const noexcept;
	void setStaticLinking(const bool inValue) noexcept;

	bool posixThreads() const noexcept;
	void setPosixThreads(const bool inValue) noexcept;

	bool windowsPrefixOutputFilename() const noexcept;
	void setWindowsPrefixOutputFilename(const bool inValue) noexcept;

	bool windowsOutputDef() const noexcept;
	void setWindowsOutputDef(const bool inValue) noexcept;

private:
	void parseStringVariables(std::string& outString);

	ProjectKind parseProjectKind(const std::string& inValue);
	StringList getDefaultCmakeDefines();
	StringList getWarningPreset();
	StringList parseWarnings(const std::string& inValue);

	const std::string& m_buildConfiguration;
	const BuildEnvironment& m_environment;

	StringList m_fileExtensions;
	StringList m_defines;
	StringList m_links;
	StringList m_staticLinks;
	StringList m_libDirs;
	StringList m_includeDirs;
	StringList m_runDependencies;
	StringList m_cmakeDefines;
	std::string m_productionDependencies;
	std::string m_productionExcludes;
	StringList m_warnings;
	StringList m_compileOptions;
	StringList m_linkerOptions;
	StringList m_macosFrameworkPaths;
	StringList m_macosFrameworks;

	StringList m_preBuildScripts;
	StringList m_postBuildScripts;

	std::string m_name;
	std::string m_outputFile;
	std::string m_cStandard;
	std::string m_cppStandard;
	StringList m_files;
	StringList m_locations;
	StringList m_locationExcludes;
	std::string m_pch;
	std::string m_runArguments;
	std::string m_linkerScript;

	ProjectKind m_kind = ProjectKind::None;
	CodeLanguage m_language = CodeLanguage::None;

	bool m_alwaysRunPostBuildScript = false;
	bool m_cmake = false;
	bool m_cmakeRecheck = false;
	bool m_dumpAssembly = false;
	bool m_objectiveCxx = false;
	bool m_rtti = true;
	bool m_runProject = false;
	bool m_staticLinking = false;
	bool m_posixThreads = true;
	bool m_includeInBuild = true;
	bool m_windowsAffixOutputFilename = true;
	bool m_windowsOutputDef = false;
};

using ProjectConfigurationList = std::vector<std::unique_ptr<ProjectConfiguration>>;
}

#endif // CHALET_PROJECT_CONFIGURATION_HPP
