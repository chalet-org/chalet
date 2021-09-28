/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/SourceTarget.hpp"

#include "State/BuildState.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
SourceTarget::SourceTarget(BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::Project),
	m_environment(inState.environment),
	m_warningsPresetString("none")
{
}

/*****************************************************************************/
bool SourceTarget::initialize()
{
	const auto& targetName = this->name();
	auto parse = [&](StringList& outList) {
		for (auto& dir : outList)
		{
			m_state.paths.replaceVariablesInPath(dir, targetName);
		}
	};

	parse(m_libDirs);
	parse(m_includeDirs);
	parse(m_runDependencies);
	parse(m_macosFrameworkPaths);
	parse(m_files);
	parse(m_locations);
	parse(m_locationExcludes);

	m_state.paths.replaceVariablesInPath(m_pch, name());

	return true;
}

/*****************************************************************************/
bool SourceTarget::validate()
{
	const auto& targetName = this->name();
	bool result = true;
	for (auto& location : m_locations)
	{
		if (String::equals(m_state.paths.intermediateDir(), location))
			continue;

		if (!Commands::pathExists(location))
		{
			Diagnostic::error("location for project target '{}' doesn't exist: {}", targetName, location);
			result = false;
		}
	}

	if (!m_pch.empty())
	{
		const auto& pch = !m_state.paths.rootDirectory().empty() ? fmt::format("{}/{}", m_state.paths.rootDirectory(), m_pch) : m_pch;
		if (!Commands::pathExists(pch))
		{
			Diagnostic::error("Precompiled header '{}' for target '{}' was not found.", pch, targetName);
			result = false;
		}
	}

	for (auto& option : m_compileOptions)
	{
		if (String::equals(option.substr(0, 2), "-W"))
		{
			Diagnostic::error("'warnings' found in 'compileOptions' (options with '-W')");
			result = false;
		}

		if (!option.empty() && option.front() != '-')
		{
			Diagnostic::error("Contents of 'compileOptions' list must begin with '-'");
			result = false;
		}
	}

	for (auto& option : m_linkerOptions)
	{
		if (String::equals(option.substr(0, 2), "-W"))
		{
			Diagnostic::error("'warnings' found in 'linkerOptions' (options with '-W'");
			result = false;
		}

		if (!option.empty() && option.front() != '-')
		{
			Diagnostic::error("Contents of 'linkerOptions' list must begin with '-'");
			result = false;
		}
	}

	if (m_invalidWarningPreset)
	{
		Diagnostic::error("Unrecognized or invalid preset for 'warnings': {}", m_warningsPresetString);
		result = false;
	}

	return result;
}

/*****************************************************************************/
bool SourceTarget::isExecutable() const noexcept
{
	return m_kind == ProjectKind::Executable;
}

bool SourceTarget::isSharedLibrary() const noexcept
{
	return m_kind == ProjectKind::SharedLibrary;
}

bool SourceTarget::isStaticLibrary() const noexcept
{
	return m_kind == ProjectKind::StaticLibrary;
}

/*****************************************************************************/
const StringList& SourceTarget::defines() const noexcept
{
	return m_defines;
}

void SourceTarget::addDefines(StringList&& inList)
{
	// -D
	List::forEach(inList, this, &SourceTarget::addDefine);
}

void SourceTarget::addDefine(std::string&& inValue)
{
	List::addIfDoesNotExist(m_defines, std::move(inValue));
}

/*****************************************************************************/
const StringList& SourceTarget::links() const noexcept
{
	return m_links;
}

void SourceTarget::addLinks(StringList&& inList)
{
	// -l
	List::forEach(inList, this, &SourceTarget::addLink);
}

void SourceTarget::addLink(std::string&& inValue)
{
	List::addIfDoesNotExist(m_links, std::move(inValue));
}

void SourceTarget::resolveLinksFromProject(const std::string& inProjectName, const bool inStaticLib)
{
	// TODO: should this behavior be separated as "projectLinks"?
	for (auto& link : m_links)
	{
		if (link != inProjectName)
			continue;

		// List::addIfDoesNotExist(m_projectStaticLinks, std::string(link));
		if (inStaticLib)
		{
			link += "-s";
		}
	}
	for (auto& link : m_staticLinks)
	{
		if (link != inProjectName)
			continue;

		List::addIfDoesNotExist(m_projectStaticLinks, std::string(link));
		if (inStaticLib)
		{
			link += "-s";
		}
	}
}

/*****************************************************************************/
const StringList& SourceTarget::projectStaticLinks() const noexcept
{
	return m_projectStaticLinks;
}

/*****************************************************************************/
const StringList& SourceTarget::staticLinks() const noexcept
{
	return m_staticLinks;
}

void SourceTarget::addStaticLinks(StringList&& inList)
{
	// -Wl,-Bstatic -l
	List::forEach(inList, this, &SourceTarget::addStaticLink);
}

void SourceTarget::addStaticLink(std::string&& inValue)
{
	List::addIfDoesNotExist(m_staticLinks, std::move(inValue));
}

/*****************************************************************************/
const StringList& SourceTarget::libDirs() const noexcept
{
	return m_libDirs;
}

void SourceTarget::addLibDirs(StringList&& inList)
{
	// -L
	List::forEach(inList, this, &SourceTarget::addLibDir);
}

void SourceTarget::addLibDir(std::string&& inValue)
{
	if (inValue.back() != '/')
		inValue += '/';

	addPathToListWithGlob(std::move(inValue), m_libDirs, GlobMatch::Folders);
}

/*****************************************************************************/
const StringList& SourceTarget::includeDirs() const noexcept
{
	return m_includeDirs;
}

void SourceTarget::addIncludeDirs(StringList&& inList)
{
	// -I
	List::forEach(inList, this, &SourceTarget::addIncludeDir);
}

void SourceTarget::addIncludeDir(std::string&& inValue)
{
	if (inValue.back() != '/')
		inValue += '/';

	addPathToListWithGlob(std::move(inValue), m_includeDirs, GlobMatch::Folders);
}

/*****************************************************************************/
const StringList& SourceTarget::runDependencies() const noexcept
{
	return m_runDependencies;
}

void SourceTarget::addRunDependencies(StringList&& inList)
{
	List::forEach(inList, this, &SourceTarget::addRunDependency);
}

void SourceTarget::addRunDependency(std::string&& inValue)
{
	// if (inValue.back() != '/')
	// 	inValue += '/'; // no!

	List::addIfDoesNotExist(m_runDependencies, std::move(inValue));
}

/*****************************************************************************/
const StringList& SourceTarget::warnings() const noexcept
{
	return m_warnings;
}

void SourceTarget::addWarnings(StringList&& inList)
{
	List::forEach(inList, this, &SourceTarget::addWarning);
	m_warningsPreset = ProjectWarnings::Custom;
}

void SourceTarget::addWarning(std::string&& inValue)
{
	if (String::equals("-W", inValue.substr(0, 2)))
	{
		Diagnostic::warn("Removing '-W' prefix from '{}'", inValue);
		inValue = inValue.substr(2);
	}

	List::addIfDoesNotExist(m_warnings, std::move(inValue));
}

/*****************************************************************************/
void SourceTarget::setWarningPreset(std::string&& inValue)
{
	m_warningsPresetString = std::move(inValue);
	m_warnings = parseWarnings(m_warningsPresetString);
}

ProjectWarnings SourceTarget::warningsPreset() const noexcept
{
	return m_warningsPreset;
}

/*****************************************************************************/
bool SourceTarget::warningsTreatedAsErrors() const noexcept
{
	return static_cast<int>(m_warningsPreset) >= static_cast<int>(ProjectWarnings::Error);
}

/*****************************************************************************/
const StringList& SourceTarget::compileOptions() const noexcept
{
	return m_compileOptions;
}

void SourceTarget::addCompileOptions(StringList&& inList)
{
	List::forEach(inList, this, &SourceTarget::addCompileOption);
}

void SourceTarget::addCompileOption(std::string&& inValue)
{
	List::addIfDoesNotExist(m_compileOptions, std::move(inValue));
}

/*****************************************************************************/
const StringList& SourceTarget::linkerOptions() const noexcept
{
	return m_linkerOptions;
}
void SourceTarget::addLinkerOptions(StringList&& inList)
{
	List::forEach(inList, this, &SourceTarget::addLinkerOption);
}
void SourceTarget::addLinkerOption(std::string&& inValue)
{
	List::addIfDoesNotExist(m_linkerOptions, std::move(inValue));
}

/*****************************************************************************/
const StringList& SourceTarget::macosFrameworkPaths() const noexcept
{
	return m_macosFrameworkPaths;
}

void SourceTarget::addMacosFrameworkPaths(StringList&& inList)
{
	// -F
	List::forEach(inList, this, &SourceTarget::addMacosFrameworkPath);
}

void SourceTarget::addMacosFrameworkPath(std::string&& inValue)
{
	if (inValue.back() != '/')
		inValue += '/';

	List::addIfDoesNotExist(m_macosFrameworkPaths, inValue);
	m_environment.addSearchPath(std::move(inValue));
}

/*****************************************************************************/
const StringList& SourceTarget::macosFrameworks() const noexcept
{
	return m_macosFrameworks;
}

void SourceTarget::addMacosFrameworks(StringList&& inList)
{
	// -framework *.framework
	List::forEach(inList, this, &SourceTarget::addMacosFramework);
}

void SourceTarget::addMacosFramework(std::string&& inValue)
{
	List::addIfDoesNotExist(m_macosFrameworks, std::move(inValue));
}

/*****************************************************************************/
const std::string& SourceTarget::outputFile() const noexcept
{
	return m_outputFile;
}

/*****************************************************************************/
const std::string& SourceTarget::outputFileNoPrefix() const noexcept
{
	return m_outputFileNoPrefix;
}

/*****************************************************************************/
const std::string& SourceTarget::cStandard() const noexcept
{
	return m_cStandard;
}

void SourceTarget::setCStandard(std::string&& inValue) noexcept
{
	m_cStandard = std::move(inValue);
}

/*****************************************************************************/
const std::string& SourceTarget::cppStandard() const noexcept
{
	return m_cppStandard;
}

void SourceTarget::setCppStandard(std::string&& inValue) noexcept
{
	m_cppStandard = std::move(inValue);
}

/*****************************************************************************/
CodeLanguage SourceTarget::language() const noexcept
{
	return m_language;
}

void SourceTarget::setLanguage(const std::string& inValue) noexcept
{
	if (String::equals("C++", inValue))
	{
		m_language = CodeLanguage::CPlusPlus;
	}
	else if (String::equals("C", inValue))
	{
		m_language = CodeLanguage::C;
	}
	else if (String::equals("Objective-C++", inValue))
	{
		m_language = CodeLanguage::CPlusPlus;
		setObjectiveCxx(true);
	}
	else if (String::equals("Objective-C", inValue))
	{
		m_language = CodeLanguage::C;
		setObjectiveCxx(true);
	}
	else
	{
		chalet_assert(false, "Invalid language for SourceTarget::setLanguage");
		m_language = CodeLanguage::None;
	}
}

/*****************************************************************************/
const StringList& SourceTarget::files() const noexcept
{
	return m_files;
}

void SourceTarget::addFiles(StringList&& inList)
{
	List::forEach(inList, this, &SourceTarget::addFile);
}

void SourceTarget::addFile(std::string&& inValue)
{
	addPathToListWithGlob(std::move(inValue), m_files, GlobMatch::Files);
}

/*****************************************************************************/
const StringList& SourceTarget::locations() const noexcept
{
	return m_locations;
}

void SourceTarget::addLocations(StringList&& inList)
{
	List::forEach(inList, this, &SourceTarget::addLocation);
}

void SourceTarget::addLocation(std::string&& inValue)
{
	addPathToListWithGlob(std::move(inValue), m_locations, GlobMatch::Folders);
}

/*****************************************************************************/
const StringList& SourceTarget::locationExcludes() const noexcept
{
	return m_locationExcludes;
}

void SourceTarget::addLocationExcludes(StringList&& inList)
{
	List::forEach(inList, this, &SourceTarget::addLocationExclude);
}

void SourceTarget::addLocationExclude(std::string&& inValue)
{
	addPathToListWithGlob(std::move(inValue), m_locationExcludes, GlobMatch::FilesAndFolders);
}

/*****************************************************************************/
const std::string& SourceTarget::pch() const noexcept
{
	return m_pch;
}

void SourceTarget::setPch(std::string&& inValue) noexcept
{
	m_pch = std::move(inValue);
}

bool SourceTarget::usesPch() const noexcept
{
	return !m_pch.empty();
}

/*****************************************************************************/
const StringList& SourceTarget::runArguments() const noexcept
{
	return m_runArguments;
}

void SourceTarget::addRunArguments(StringList&& inList)
{
	List::forEach(inList, this, &SourceTarget::addRunArgument);
}

void SourceTarget::addRunArgument(std::string&& inValue)
{
	m_runArguments.emplace_back(std::move(inValue));
}

/*****************************************************************************/
const std::string& SourceTarget::linkerScript() const noexcept
{
	return m_linkerScript;
}

void SourceTarget::setLinkerScript(std::string&& inValue) noexcept
{
	m_linkerScript = std::move(inValue);
}

/*****************************************************************************/
const std::string& SourceTarget::windowsApplicationManifest() const noexcept
{
	return m_windowsApplicationManifest;
}

void SourceTarget::setWindowsApplicationManifest(std::string&& inValue) noexcept
{
	m_windowsApplicationManifest = std::move(inValue);
}

/*****************************************************************************/
bool SourceTarget::windowsApplicationManifestGenerationEnabled() const noexcept
{
	return m_windowsApplicationManifestGenerationEnabled;
}
void SourceTarget::setWindowsApplicationManifestGenerationEnabled(const bool inValue) noexcept
{
	m_windowsApplicationManifestGenerationEnabled = inValue;
}

/*****************************************************************************/
const std::string& SourceTarget::windowsApplicationIcon() const noexcept
{
	return m_windowsApplicationIcon;
}

void SourceTarget::setWindowsApplicationIcon(std::string&& inValue) noexcept
{
	m_windowsApplicationIcon = std::move(inValue);
}

/*****************************************************************************/
ProjectKind SourceTarget::kind() const noexcept
{
	return m_kind;
}

void SourceTarget::setKind(const ProjectKind inValue) noexcept
{
	m_kind = inValue;
}

void SourceTarget::setKind(const std::string& inValue)
{
	m_kind = parseProjectKind(inValue);
}

/*****************************************************************************/
ThreadType SourceTarget::threadType() const noexcept
{
	return m_threadType;
}

void SourceTarget::setThreadType(const ThreadType inValue) noexcept
{
	m_threadType = inValue;
}

void SourceTarget::setThreadType(const std::string& inValue)
{
	m_threadType = parseThreadType(inValue);
}

/*****************************************************************************/
WindowsSubSystem SourceTarget::windowsSubSystem() const noexcept
{
	return m_windowsSubSystem;
}

void SourceTarget::setWindowsSubSystem(const WindowsSubSystem inValue) noexcept
{
	m_windowsSubSystem = inValue;
}

void SourceTarget::setWindowsSubSystem(const std::string& inValue)
{
	m_windowsSubSystem = parseWindowsSubSystem(inValue);
}

/*****************************************************************************/
WindowsEntryPoint SourceTarget::windowsEntryPoint() const noexcept
{
	return m_windowsEntryPoint;
}

void SourceTarget::setWindowsEntryPoint(const WindowsEntryPoint inValue) noexcept
{
	m_windowsEntryPoint = inValue;
}

void SourceTarget::setWindowsEntryPoint(const std::string& inValue)
{
	m_windowsEntryPoint = parseWindowsEntryPoint(inValue);
}

/*****************************************************************************/
bool SourceTarget::objectiveCxx() const noexcept
{
	return m_objectiveCxx;
}

void SourceTarget::setObjectiveCxx(const bool inValue) noexcept
{
	m_objectiveCxx = inValue;
}

/*****************************************************************************/
bool SourceTarget::rtti() const noexcept
{
	return m_rtti;
}

void SourceTarget::setRtti(const bool inValue) noexcept
{
	m_rtti = inValue;
}

bool SourceTarget::exceptions() const noexcept
{
	return m_exceptions;
}
void SourceTarget::setExceptions(const bool inValue) noexcept
{
	m_exceptions = inValue;
}

/*****************************************************************************/
bool SourceTarget::runProject() const noexcept
{
	return m_runProject;
}

void SourceTarget::setRunProject(const bool inValue) noexcept
{
	m_runProject = inValue;
}

/*****************************************************************************/
bool SourceTarget::staticLinking() const noexcept
{
	return m_staticLinking;
}

void SourceTarget::setStaticLinking(const bool inValue) noexcept
{
	m_staticLinking = inValue;
}

/*****************************************************************************/
// TODO: string prefix (lib) / suffix (-s) control
bool SourceTarget::windowsPrefixOutputFilename() const noexcept
{
	bool staticLib = m_kind == ProjectKind::StaticLibrary;
	return m_windowsPrefixOutputFilename || staticLib;
}

void SourceTarget::setWindowsPrefixOutputFilename(const bool inValue) noexcept
{
	m_windowsPrefixOutputFilename = inValue;
	m_setWindowsPrefixOutputFilename = true;
}

/*****************************************************************************/
bool SourceTarget::windowsOutputDef() const noexcept
{
	return m_windowsOutputDef;
}
void SourceTarget::setWindowsOutputDef(const bool inValue) noexcept
{
	m_windowsOutputDef = inValue;
}

/*****************************************************************************/
void SourceTarget::addPathToListWithGlob(std::string&& inValue, StringList& outList, const GlobMatch inSettings)
{
	if (String::contains('*', inValue))
	{
		Commands::forEachGlobMatch(inValue, inSettings, [&](const fs::path& inPath) {
			auto path = inPath.string();
			Path::sanitize(path);

			List::addIfDoesNotExist(outList, std::move(path));
		});
	}
	else
	{
		List::addIfDoesNotExist(outList, std::move(inValue));
	}
}

/*****************************************************************************/
ProjectKind SourceTarget::parseProjectKind(const std::string& inValue)
{
	if (String::equals("executable", inValue))
		return ProjectKind::Executable;

	if (String::equals("staticLibrary", inValue))
		return ProjectKind::StaticLibrary;

	if (String::equals("sharedLibrary", inValue))
		return ProjectKind::SharedLibrary;

	return ProjectKind::None;
}

/*****************************************************************************/
ThreadType SourceTarget::parseThreadType(const std::string& inValue)
{
	if (String::equals("auto", inValue))
		return ThreadType::Auto;

	if (String::equals("posix", inValue))
		return ThreadType::Posix;

	return ThreadType::None;
}

/*****************************************************************************/
WindowsSubSystem SourceTarget::parseWindowsSubSystem(const std::string& inValue)
{
	if (String::equals("console", inValue))
		return WindowsSubSystem::Console;

	if (String::equals("windows", inValue))
		return WindowsSubSystem::Windows;

	if (String::equals("bootApplication", inValue))
		return WindowsSubSystem::BootApplication;

	if (String::equals("native", inValue))
		return WindowsSubSystem::Native;

	if (String::equals("posix", inValue))
		return WindowsSubSystem::Posix;

	if (String::equals("efiApplication", inValue))
		return WindowsSubSystem::EfiApplication;

	if (String::equals("efiBootServer", inValue))
		return WindowsSubSystem::EfiBootServiceDriver;

	if (String::equals("efiRom", inValue))
		return WindowsSubSystem::EfiRom;

	if (String::equals("efiRuntimeDriver", inValue))
		return WindowsSubSystem::EfiRuntimeDriver;

	return WindowsSubSystem::None;
}

/*****************************************************************************/
WindowsEntryPoint SourceTarget::parseWindowsEntryPoint(const std::string& inValue)
{
	if (String::equals("main", inValue))
		return WindowsEntryPoint::Main;

	if (String::equals("wmain", inValue))
		return WindowsEntryPoint::MainUnicode;

	if (String::equals("WinMain", inValue))
		return WindowsEntryPoint::WinMain;

	if (String::equals("wWinMain", inValue))
		return WindowsEntryPoint::WinMainUnicode;

	if (String::equals("DllMain", inValue))
		return WindowsEntryPoint::DllMain;

	return WindowsEntryPoint::None;
}

/*****************************************************************************/
void SourceTarget::parseOutputFilename(const CompilerConfig& inConfig) noexcept
{
	const auto& projectName = name();
	chalet_assert(!projectName.empty(), "parseOutputFilename: name is blank");

	bool staticLib = m_kind == ProjectKind::StaticLibrary;

#if defined(CHALET_WIN32)
	std::string executableExtension{ ".exe" };
	std::string libraryExtension{ ".dll" };
#elif defined(CHALET_MACOS)
	std::string executableExtension;
	std::string libraryExtension{ ".dylib" };
#else
	std::string executableExtension;
	std::string libraryExtension{ ".so" };
#endif

	if (staticLib)
	{
#if defined(CHALET_WIN32)
		if (inConfig.isMsvc() || inConfig.isWindowsClang())
			libraryExtension = "-s.lib";
		else
#endif
			libraryExtension = "-s.a";
	}

	switch (m_kind)
	{
		case ProjectKind::Executable: {
			m_outputFile = projectName + executableExtension;
			m_outputFileNoPrefix = m_outputFile;
			break;
		}
		case ProjectKind::SharedLibrary:
		case ProjectKind::StaticLibrary: {
			if (!windowsPrefixOutputFilename() || (inConfig.isMsvc() && !m_setWindowsPrefixOutputFilename) || inConfig.isWindowsClang())
			{
				m_outputFile = projectName + libraryExtension;
				m_outputFileNoPrefix = m_outputFile;
			}
			else
			{
				m_outputFileNoPrefix = projectName + libraryExtension;
				m_outputFile = "lib" + m_outputFileNoPrefix;
			}
			break;
		}
		default:
			break;
	}
}

/*****************************************************************************/
// TODO: These will need numerous discussions as to how they can be categorized
//
StringList SourceTarget::parseWarnings(const std::string& inValue)
{
	StringList ret;

	if (String::equals("none", inValue))
	{
		m_warningsPreset = ProjectWarnings::None;
		return ret;
	}

	ret.emplace_back("all");
	if (String::equals("minimal", inValue))
	{
		m_warningsPreset = ProjectWarnings::Minimal;
		return ret;
	}

	ret.emplace_back("extra");
	if (String::equals("extra", inValue))
	{
		m_warningsPreset = ProjectWarnings::Extra;
		return ret;
	}

	ret.emplace_back("error");
	if (String::equals("error", inValue))
	{
		m_warningsPreset = ProjectWarnings::Error;
		return ret;
	}

	ret.emplace_back("pedantic");
	// ret.emplace_back("pedantic-errors"); // Not on OSX?

	if (String::equals("pedantic", inValue))
	{
		m_warningsPreset = ProjectWarnings::Pedantic;
		return ret;
	}

	ret.emplace_back("unused");
	ret.emplace_back("cast-align");
	ret.emplace_back("double-promotion");
	ret.emplace_back("format=2");
	ret.emplace_back("missing-declarations");
	ret.emplace_back("missing-include-dirs");
	ret.emplace_back("non-virtual-dtor");
	ret.emplace_back("redundant-decls");
	ret.emplace_back("odr");

	if (String::equals("strict", inValue))
	{
		m_warningsPreset = ProjectWarnings::Strict;
		return ret;
	}

	ret.emplace_back("unreachable-code"); // clang only
	ret.emplace_back("shadow");

	if (String::equals("strictPedantic", inValue))
	{
		m_warningsPreset = ProjectWarnings::StrictPedantic;
		return ret;
	}

	ret.emplace_back("noexcept");
	ret.emplace_back("undef");
	ret.emplace_back("conversion");
	ret.emplace_back("cast-qual");
	ret.emplace_back("float-equal");
	ret.emplace_back("inline");
	ret.emplace_back("old-style-cast");
	ret.emplace_back("strict-null-sentinel");
	ret.emplace_back("overloaded-virtual");
	ret.emplace_back("sign-conversion");
	ret.emplace_back("sign-promo");

	if (String::equals("veryStrict", inValue))
	{
		m_warningsPreset = ProjectWarnings::VeryStrict;
		return ret;
	}

	// More?
	// can't be ignored in GCC 10.2.0, so best not to use it at all
	// ret.emplace_back("switch-default");

	m_invalidWarningPreset = true;

	return ret;
}
}
