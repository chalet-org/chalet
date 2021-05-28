/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/ProjectTarget.hpp"

#include "Libraries/Format.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ProjectTarget::ProjectTarget(const BuildState& inState) :
	IBuildTarget(inState, TargetType::Project)
{
	StringList exts = {
		"cpp",
		"cc",
		"cxx",
		"c++",
		"c",
		"mm",
		"m",
		"rc"
	};
	addFileExtensions(exts);
}

/*****************************************************************************/
bool ProjectTarget::isExecutable() const noexcept
{
	return m_kind == ProjectKind::ConsoleApplication || m_kind == ProjectKind::DesktopApplication;
}

bool ProjectTarget::isSharedLibrary() const noexcept
{
	return m_kind == ProjectKind::SharedLibrary;
}

bool ProjectTarget::isStaticLibrary() const noexcept
{
	return m_kind == ProjectKind::StaticLibrary;
}

/*****************************************************************************/
const StringList& ProjectTarget::fileExtensions() const noexcept
{
	return m_fileExtensions;
}

void ProjectTarget::addFileExtensions(StringList& inList)
{
	List::forEach(inList, this, &ProjectTarget::addFileExtension);
}

void ProjectTarget::addFileExtension(std::string& inValue)
{
	if (!inValue.empty() && inValue.front() != '.')
		inValue = "." + inValue;

	List::addIfDoesNotExist(m_fileExtensions, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectTarget::defines() const noexcept
{
	return m_defines;
}

void ProjectTarget::addDefines(StringList& inList)
{
	// -D
	List::forEach(inList, this, &ProjectTarget::addDefine);
}

void ProjectTarget::addDefine(std::string& inValue)
{
	List::addIfDoesNotExist(m_defines, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectTarget::links() const noexcept
{
	return m_links;
}

void ProjectTarget::addLinks(StringList& inList)
{
	// -l
	List::forEach(inList, this, &ProjectTarget::addLink);
}

void ProjectTarget::addLink(std::string& inValue)
{
	List::addIfDoesNotExist(m_links, std::move(inValue));
}

void ProjectTarget::resolveLinksFromProject(const std::string& inProjectName, const bool inStaticLib)
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
const StringList& ProjectTarget::projectStaticLinks() const noexcept
{
	return m_projectStaticLinks;
}

/*****************************************************************************/
const StringList& ProjectTarget::staticLinks() const noexcept
{
	return m_staticLinks;
}

void ProjectTarget::addStaticLinks(StringList& inList)
{
	// -Wl,-Bstatic -l
	List::forEach(inList, this, &ProjectTarget::addStaticLink);
}

void ProjectTarget::addStaticLink(std::string& inValue)
{
	List::addIfDoesNotExist(m_staticLinks, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectTarget::libDirs() const noexcept
{
	return m_libDirs;
}

void ProjectTarget::addLibDirs(StringList& inList)
{
	// -L
	List::forEach(inList, this, &ProjectTarget::addLibDir);
}

void ProjectTarget::addLibDir(std::string& inValue)
{
	if (inValue.back() != '/')
		inValue += '/';

	// TODO: check other places this can be done
	parseStringVariables(inValue);
	Path::sanitize(inValue);

	List::addIfDoesNotExist(m_libDirs, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectTarget::includeDirs() const noexcept
{
	return m_includeDirs;
}

void ProjectTarget::addIncludeDirs(StringList& inList)
{
	// -I
	List::forEach(inList, this, &ProjectTarget::addIncludeDir);
}

void ProjectTarget::addIncludeDir(std::string& inValue)
{
	if (inValue.back() != '/')
		inValue += '/';

	parseStringVariables(inValue);
	Path::sanitize(inValue);

	List::addIfDoesNotExist(m_includeDirs, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectTarget::runDependencies() const noexcept
{
	return m_runDependencies;
}

void ProjectTarget::addRunDependencies(StringList& inList)
{
	List::forEach(inList, this, &ProjectTarget::addRunDependency);
}

void ProjectTarget::addRunDependency(std::string& inValue)
{
	if (inValue.back() != '/')
		inValue += '/';

	parseStringVariables(inValue);
	Path::sanitize(inValue);

	List::addIfDoesNotExist(m_runDependencies, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectTarget::warnings() const noexcept
{
	return m_warnings;
}

void ProjectTarget::addWarnings(StringList& inList)
{
	List::forEach(inList, this, &ProjectTarget::addWarning);
	m_warningsPreset = ProjectWarnings::Custom;
}

void ProjectTarget::addWarning(std::string& inValue)
{
	if (String::equals(inValue.substr(0, 2), "-W"))
	{
		Diagnostic::warn(fmt::format("Removing '-W' prefix from '{}'", inValue));
		inValue = inValue.substr(2);
	}

	List::addIfDoesNotExist(m_warnings, std::move(inValue));
}

void ProjectTarget::setWarningPreset(const std::string& inValue)
{
	m_warnings = parseWarnings(inValue);
}

ProjectWarnings ProjectTarget::warningsPreset() const noexcept
{
	return m_warningsPreset;
}

/*****************************************************************************/
bool ProjectTarget::warningsTreatedAsErrors() const noexcept
{
	return static_cast<int>(m_warningsPreset) >= static_cast<int>(ProjectWarnings::Error);
}

/*****************************************************************************/
const StringList& ProjectTarget::compileOptions() const noexcept
{
	return m_compileOptions;
}

void ProjectTarget::addCompileOptions(StringList& inList)
{
	List::forEach(inList, this, &ProjectTarget::addCompileOption);
}

void ProjectTarget::addCompileOption(std::string& inValue)
{
	if (String::equals(inValue.substr(0, 2), "-W"))
	{
		Diagnostic::errorAbort("'warnings' found in 'compileOptions' (options with '-W')");
		return;
	}

	if (!inValue.empty() && inValue.front() != '-')
	{
		Diagnostic::errorAbort("Contents of 'compileOptions' list must begin with '-'");
		return;
	}

	List::addIfDoesNotExist(m_compileOptions, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectTarget::linkerOptions() const noexcept
{
	return m_linkerOptions;
}
void ProjectTarget::addLinkerOptions(StringList& inList)
{
	List::forEach(inList, this, &ProjectTarget::addLinkerOption);
}
void ProjectTarget::addLinkerOption(std::string& inValue)
{
	if (String::equals(inValue.substr(0, 2), "-W"))
	{
		Diagnostic::errorAbort("'warnings' found in 'linkerOptions' (options with '-W'");
		return;
	}

	if (!inValue.empty() && inValue.front() != '-')
	{
		Diagnostic::errorAbort("Contents of 'linkerOptions' list must begin with '-'");
		return;
	}

	List::addIfDoesNotExist(m_linkerOptions, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectTarget::macosFrameworkPaths() const noexcept
{
	return m_macosFrameworkPaths;
}

void ProjectTarget::addMacosFrameworkPaths(StringList& inList)
{
	// -F
	List::forEach(inList, this, &ProjectTarget::addMacosFrameworkPath);
}

void ProjectTarget::addMacosFrameworkPath(std::string& inValue)
{
	if (inValue.back() != '/')
		inValue += '/';

	parseStringVariables(inValue);
	Path::sanitize(inValue);

	List::addIfDoesNotExist(m_macosFrameworkPaths, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectTarget::macosFrameworks() const noexcept
{
	return m_macosFrameworks;
}

void ProjectTarget::addMacosFrameworks(StringList& inList)
{
	// -framework *.framework
	List::forEach(inList, this, &ProjectTarget::addMacosFramework);
}

void ProjectTarget::addMacosFramework(std::string& inValue)
{
	List::addIfDoesNotExist(m_macosFrameworks, std::move(inValue));
}

/*****************************************************************************/
const std::string& ProjectTarget::outputFile() const noexcept
{
	return m_outputFile;
}

/*****************************************************************************/
const std::string& ProjectTarget::cStandard() const noexcept
{
	return m_cStandard;
}

void ProjectTarget::setCStandard(const std::string& inValue) noexcept
{
	m_cStandard = inValue;
}

/*****************************************************************************/
const std::string& ProjectTarget::cppStandard() const noexcept
{
	return m_cppStandard;
}

void ProjectTarget::setCppStandard(const std::string& inValue) noexcept
{
	m_cppStandard = inValue;
}

/*****************************************************************************/
CodeLanguage ProjectTarget::language() const noexcept
{
	return m_language;
}

void ProjectTarget::setLanguage(const std::string& inValue) noexcept
{
	if (String::equals(inValue, "C++"))
	{
		m_language = CodeLanguage::CPlusPlus;
	}
	else if (String::equals("C", inValue))
	{
		m_language = CodeLanguage::C;
	}
	else
	{
		chalet_assert(false, "Invalid language for ProjectTarget::setLanguage");
		m_language = CodeLanguage::None;
	}
}

/*****************************************************************************/
const StringList& ProjectTarget::files() const noexcept
{
	return m_files;
}

void ProjectTarget::addFiles(StringList& inList)
{
	List::forEach(inList, this, &ProjectTarget::addFile);
}

void ProjectTarget::addFile(std::string& inValue)
{
	parseStringVariables(inValue);
	Path::sanitize(inValue);
	List::addIfDoesNotExist(m_files, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectTarget::locations() const noexcept
{
	return m_locations;
}

void ProjectTarget::addLocations(StringList& inList)
{
	List::forEach(inList, this, &ProjectTarget::addLocation);
}

void ProjectTarget::addLocation(std::string& inValue)
{
	parseStringVariables(inValue);
	Path::sanitize(inValue);

	List::addIfDoesNotExist(m_locations, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectTarget::locationExcludes() const noexcept
{
	return m_locationExcludes;
}

void ProjectTarget::addLocationExcludes(StringList& inList)
{
	List::forEach(inList, this, &ProjectTarget::addLocationExclude);
}

void ProjectTarget::addLocationExclude(std::string& inValue)
{
	parseStringVariables(inValue);
	Path::sanitize(inValue);

	List::addIfDoesNotExist(m_locationExcludes, std::move(inValue));
}

/*****************************************************************************/
const std::string& ProjectTarget::pch() const noexcept
{
	return m_pch;
}

void ProjectTarget::setPch(const std::string& inValue) noexcept
{
	m_pch = inValue;

	parseStringVariables(m_pch);
	Path::sanitize(m_pch);

	std::string path = String::getPathFolder(m_pch);
	addLocation(path);
}

bool ProjectTarget::usesPch() const noexcept
{
	return !m_pch.empty();
}

/*****************************************************************************/
const StringList& ProjectTarget::runArguments() const noexcept
{
	return m_runArguments;
}

void ProjectTarget::addRunArguments(StringList& inList)
{
	List::forEach(inList, this, &ProjectTarget::addRunArgument);
}

void ProjectTarget::addRunArgument(std::string& inValue)
{
	m_runArguments.push_back(std::move(inValue));
}

/*****************************************************************************/
const std::string& ProjectTarget::linkerScript() const noexcept
{
	return m_linkerScript;
}

void ProjectTarget::setLinkerScript(const std::string& inValue) noexcept
{
	m_linkerScript = inValue;
}

/*****************************************************************************/
ProjectKind ProjectTarget::kind() const noexcept
{
	return m_kind;
}

void ProjectTarget::setKind(const ProjectKind inValue) noexcept
{
	m_kind = inValue;
}

void ProjectTarget::setKind(const std::string& inValue)
{
	m_kind = parseProjectKind(inValue);
}

/*****************************************************************************/
bool ProjectTarget::dumpAssembly() const noexcept
{
	return m_dumpAssembly;
}

void ProjectTarget::setDumpAssembly(const bool inValue) noexcept
{
	m_dumpAssembly = inValue;
}

/*****************************************************************************/
bool ProjectTarget::objectiveCxx() const noexcept
{
	return m_objectiveCxx;
}
void ProjectTarget::setObjectiveCxx(const bool inValue) noexcept
{
	m_objectiveCxx = inValue;
}

/*****************************************************************************/
bool ProjectTarget::rtti() const noexcept
{
	return m_rtti;
}

void ProjectTarget::setRtti(const bool inValue) noexcept
{
	m_rtti = inValue;
}

/*****************************************************************************/
bool ProjectTarget::runProject() const noexcept
{
	return m_runProject;
}

void ProjectTarget::setRunProject(const bool inValue) noexcept
{
	m_runProject = inValue;
}

/*****************************************************************************/
bool ProjectTarget::staticLinking() const noexcept
{
	return m_staticLinking;
}

void ProjectTarget::setStaticLinking(const bool inValue) noexcept
{
	m_staticLinking = inValue;
}

/*****************************************************************************/
bool ProjectTarget::posixThreads() const noexcept
{
	return m_posixThreads;
}

void ProjectTarget::setPosixThreads(const bool inValue) noexcept
{
	m_posixThreads = inValue;
}

/*****************************************************************************/
// TODO: string prefix (lib) / suffix (-s) control
bool ProjectTarget::windowsPrefixOutputFilename() const noexcept
{
	bool staticLib = m_kind == ProjectKind::StaticLibrary;
	return m_windowsPrefixOutputFilename || staticLib;
}

void ProjectTarget::setWindowsPrefixOutputFilename(const bool inValue) noexcept
{
	m_windowsPrefixOutputFilename = inValue;
	m_setWindowsPrefixOutputFilename = true;
}

bool ProjectTarget::setWindowsPrefixOutputFilename() const noexcept
{
	return m_setWindowsPrefixOutputFilename;
}

/*****************************************************************************/
bool ProjectTarget::windowsOutputDef() const noexcept
{
	return m_windowsOutputDef;
}
void ProjectTarget::setWindowsOutputDef(const bool inValue) noexcept
{
	m_windowsOutputDef = inValue;
}

/*****************************************************************************/
ProjectKind ProjectTarget::parseProjectKind(const std::string& inValue)
{
	if (String::equals("staticLibrary", inValue))
		return ProjectKind::StaticLibrary;

	if (String::equals("sharedLibrary", inValue))
		return ProjectKind::SharedLibrary;

	if (String::equals("consoleApplication", inValue))
		return ProjectKind::ConsoleApplication;

	if (String::equals("desktopApplication", inValue))
		return ProjectKind::DesktopApplication;

	return ProjectKind::None;
}

/*****************************************************************************/
void ProjectTarget::parseOutputFilename(const CompilerConfig& inConfig) noexcept
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
		if (inConfig.isMsvc())
			libraryExtension = "-s.lib";
		else
#endif
			libraryExtension = "-s.a";
	}

	switch (m_kind)
	{
		case ProjectKind::ConsoleApplication:
		case ProjectKind::DesktopApplication: {
			m_outputFile = projectName + executableExtension;
			break;
		}
		case ProjectKind::SharedLibrary:
		case ProjectKind::StaticLibrary: {
			if (!windowsPrefixOutputFilename() || (inConfig.isMsvc() && !m_setWindowsPrefixOutputFilename))
			{
				m_outputFile = projectName + libraryExtension;
			}
			else
			{
				m_outputFile = "lib" + projectName + libraryExtension;
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
StringList ProjectTarget::parseWarnings(const std::string& inValue)
{
	StringList ret;

	if (String::equals("none", inValue))
	{
		m_warningsPreset = ProjectWarnings::None;
		return ret;
	}

	ret.push_back("all");
	if (String::equals("minimal", inValue))
	{
		m_warningsPreset = ProjectWarnings::Minimal;
		return ret;
	}

	ret.push_back("extra");
	if (String::equals("extra", inValue))
	{
		m_warningsPreset = ProjectWarnings::Extra;
		return ret;
	}

	ret.push_back("error");
	if (String::equals("error", inValue))
	{
		m_warningsPreset = ProjectWarnings::Error;
		return ret;
	}

	ret.push_back("pedantic");
	// ret.push_back("pedantic-errors"); // Not on OSX?

	if (String::equals("pedantic", inValue))
	{
		m_warningsPreset = ProjectWarnings::Pedantic;
		return ret;
	}

	ret.push_back("unused");
	ret.push_back("cast-align");
	ret.push_back("double-promotion");
	ret.push_back("format=2");
	ret.push_back("missing-declarations");
	ret.push_back("missing-include-dirs");
	ret.push_back("non-virtual-dtor");
	ret.push_back("redundant-decls");
	ret.push_back("odr");

	if (String::equals("strict", inValue))
	{
		m_warningsPreset = ProjectWarnings::Strict;
		return ret;
	}

	ret.push_back("unreachable-code"); // clang only
	ret.push_back("shadow");

	if (String::equals("strictPedantic", inValue))
	{
		m_warningsPreset = ProjectWarnings::StrictPedantic;
		return ret;
	}

	ret.push_back("noexcept");
	ret.push_back("undef");
	ret.push_back("conversion");
	ret.push_back("cast-qual");
	ret.push_back("float-equal");
	ret.push_back("inline");
	ret.push_back("old-style-cast");
	ret.push_back("strict-null-sentinel");
	ret.push_back("overloaded-virtual");
	ret.push_back("sign-conversion");
	ret.push_back("sign-promo");

	if (String::equals("veryStrict", inValue))
	{
		m_warningsPreset = ProjectWarnings::VeryStrict;
		return ret;
	}

	// More?
	// can't be ignored in GCC 10.2.0, so best not to use it at all
	// ret.push_back("switch-default");

	Diagnostic::errorAbort(fmt::format("Unrecognized or invalid value for 'warnings': {}", inValue));

	return ret;
}
}
