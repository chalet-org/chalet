/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SOURCE_TARGET_HPP
#define CHALET_SOURCE_TARGET_HPP

#include "Compile/CodeLanguage.hpp"
#include "State/ProjectKind.hpp"
#include "State/ProjectWarningPresets.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/ThreadType.hpp"
#include "State/WindowsEntryPoint.hpp"
#include "State/WindowsSubSystem.hpp"
#include "Utility/GlobMatch.hpp"

namespace chalet
{
class BuildState;

struct SourceTarget final : public IBuildTarget
{
	explicit SourceTarget(BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;

	bool isExecutable() const noexcept;
	bool isSharedLibrary() const noexcept;
	bool isStaticLibrary() const noexcept;

	const StringList& defines() const noexcept;
	void addDefines(StringList&& inList);
	void addDefine(std::string&& inValue);

	const StringList& links() const noexcept;
	void addLinks(StringList&& inList);
	void addLink(std::string&& inValue);
	bool resolveLinksFromProject(const std::vector<BuildTarget>& inTargets, const std::string& inInputFile);

	const StringList& projectStaticLinks() const noexcept;

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
	bool warningsTreatedAsErrors() const noexcept;

	const StringList& compileOptions() const noexcept;
	void addCompileOptions(std::string&& inValue);

	const StringList& linkerOptions() const noexcept;
	void addLinkerOptions(std::string&& inValue);

	const StringList& macosFrameworkPaths() const noexcept;
	void addMacosFrameworkPaths(StringList&& inList);
	void addMacosFrameworkPath(std::string&& inValue);

	const StringList& macosFrameworks() const noexcept;
	void addMacosFrameworks(StringList&& inList);
	void addMacosFramework(std::string&& inValue);

	//
	void parseOutputFilename() noexcept;

	const std::string& outputFile() const noexcept;
	const std::string& outputFileNoPrefix() const noexcept;

	CodeLanguage language() const noexcept;
	void setLanguage(const std::string& inValue) noexcept;

	const std::string& cStandard() const noexcept;
	void setCStandard(std::string&& inValue) noexcept;

	const std::string& cppStandard() const noexcept;
	void setCppStandard(std::string&& inValue) noexcept;

	const StringList& files() const noexcept;
	void addFiles(StringList&& inList);
	void addFile(std::string&& inValue);

	const StringList& locations() const noexcept;
	void addLocations(StringList&& inList);
	void addLocation(std::string&& inValue);

	const StringList& locationExcludes() const noexcept;
	void addLocationExcludes(StringList&& inList);
	void addLocationExclude(std::string&& inValue);

	const std::string& pch() const noexcept;
	void setPch(std::string&& inValue) noexcept;
	bool usesPch() const noexcept;

	const std::string& linkerScript() const noexcept;
	void setLinkerScript(std::string&& inValue) noexcept;

	const std::string& windowsApplicationManifest() const noexcept;
	void setWindowsApplicationManifest(std::string&& inValue) noexcept;

	bool windowsApplicationManifestGenerationEnabled() const noexcept;
	void setWindowsApplicationManifestGenerationEnabled(const bool inValue) noexcept;

	const std::string& windowsApplicationIcon() const noexcept;
	void setWindowsApplicationIcon(std::string&& inValue) noexcept;

	//
	ProjectKind kind() const noexcept;
	void setKind(const ProjectKind inValue) noexcept;
	void setKind(const std::string& inValue);

	ThreadType threadType() const noexcept;
	void setThreadType(const ThreadType inValue) noexcept;
	void setThreadType(const std::string& inValue);

	WindowsSubSystem windowsSubSystem() const noexcept;
	void setWindowsSubSystem(const WindowsSubSystem inValue) noexcept;
	void setWindowsSubSystem(const std::string& inValue);

	WindowsEntryPoint windowsEntryPoint() const noexcept;
	void setWindowsEntryPoint(const WindowsEntryPoint inValue) noexcept;
	void setWindowsEntryPoint(const std::string& inValue);

	bool cppModules() const noexcept;
	void setCppModules(const bool inValue) noexcept;

	bool cppCoroutines() const noexcept;
	void setCppCoroutines(const bool inValue) noexcept;

	bool cppConcepts() const noexcept;
	void setCppConcepts(const bool inValue) noexcept;

	bool objectiveCxx() const noexcept;
	void setObjectiveCxx(const bool inValue) noexcept;

	bool rtti() const noexcept;
	void setRtti(const bool inValue) noexcept;

	bool exceptions() const noexcept;
	void setExceptions(const bool inValue) noexcept;

	bool staticLinking() const noexcept;
	void setStaticLinking(const bool inValue) noexcept;

	bool windowsPrefixOutputFilename() const noexcept;

	bool windowsOutputDef() const noexcept;
	void setWindowsOutputDef(const bool inValue) noexcept;

private:
	ProjectKind parseProjectKind(const std::string& inValue);
	ThreadType parseThreadType(const std::string& inValue);
	WindowsSubSystem parseWindowsSubSystem(const std::string& inValue);
	WindowsEntryPoint parseWindowsEntryPoint(const std::string& inValue);
	StringList getWarningPreset();
	StringList parseWarnings(const std::string& inValue);
	StringList parseCommandLineOptions(std::string inString) const;

	StringList m_fileExtensions;
	StringList m_defines;
	StringList m_links;
	StringList m_projectStaticLinks;
	StringList m_staticLinks;
	StringList m_libDirs;
	StringList m_includeDirs;
	StringList m_warnings;
	StringList m_compileOptions;
	StringList m_linkerOptions;
	StringList m_macosFrameworkPaths;
	StringList m_macosFrameworks;
	StringList m_files;
	StringList m_locations;
	StringList m_locationExcludes;

	std::string m_productionDependencies;
	std::string m_productionExcludes;
	std::string m_warningsPresetString;
	std::string m_outputFile;
	std::string m_outputFileNoPrefix;
	std::string m_cStandard;
	std::string m_cppStandard;
	std::string m_pch;
	std::string m_compileOptionsRaw;
	std::string m_linkerOptionsRaw;
	std::string m_linkerScript;
	std::string m_windowsApplicationManifest;
	std::string m_windowsApplicationIcon;

	ProjectKind m_kind = ProjectKind::None;
	CodeLanguage m_language = CodeLanguage::None;
	ProjectWarningPresets m_warningsPreset = ProjectWarningPresets::None;
	ThreadType m_threadType = ThreadType::Auto;
	WindowsSubSystem m_windowsSubSystem = WindowsSubSystem::Console;
	WindowsEntryPoint m_windowsEntryPoint = WindowsEntryPoint::Main;

	bool m_cppModules = false;
	bool m_cppCoroutines = false;
	bool m_cppConcepts = false;
	bool m_objectiveCxx = false;
	bool m_rtti = true;
	bool m_exceptions = true;
	bool m_staticLinking = false;
	bool m_posixThreads = true;
	bool m_invalidWarningPreset = false;
	bool m_windowsApplicationManifestGenerationEnabled = true;
	bool m_setWindowsPrefixOutputFilename = false;
	bool m_windowsOutputDef = false;
};
}

#endif // CHALET_SOURCE_TARGET_HPP
