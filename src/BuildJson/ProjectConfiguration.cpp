/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/ProjectConfiguration.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ProjectConfiguration::ProjectConfiguration(const std::string& inBuildConfig, const BuildEnvironment& inEnvironment) :
	m_buildConfiguration(inBuildConfig),
	m_environment(inEnvironment),
	m_cmakeDefines(getDefaultCmakeDefines())
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
bool ProjectConfiguration::isExecutable() const noexcept
{
	return m_kind == ProjectKind::ConsoleApplication || m_kind == ProjectKind::DesktopApplication;
}

bool ProjectConfiguration::isSharedLibrary() const noexcept
{
	return m_kind == ProjectKind::SharedLibrary;
}

bool ProjectConfiguration::isStaticLibrary() const noexcept
{
	return m_kind == ProjectKind::StaticLibrary;
}

/*****************************************************************************/
const StringList& ProjectConfiguration::fileExtensions() const noexcept
{
	return m_fileExtensions;
}

void ProjectConfiguration::addFileExtensions(StringList& inList)
{
	List::forEach(inList, this, &ProjectConfiguration::addFileExtension);
}

void ProjectConfiguration::addFileExtension(std::string& inValue)
{
	if (!inValue.empty() && inValue.front() != '.')
		inValue = "." + inValue;

	List::addIfDoesNotExist(m_fileExtensions, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectConfiguration::defines() const noexcept
{
	return m_defines;
}

void ProjectConfiguration::addDefines(StringList& inList)
{
	// -D
	List::forEach(inList, this, &ProjectConfiguration::addDefine);
}

void ProjectConfiguration::addDefine(std::string& inValue)
{
	List::addIfDoesNotExist(m_defines, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectConfiguration::links() const noexcept
{
	return m_links;
}

void ProjectConfiguration::addLinks(StringList& inList)
{
	// -l
	List::forEach(inList, this, &ProjectConfiguration::addLink);
}

void ProjectConfiguration::addLink(std::string& inValue)
{
	List::addIfDoesNotExist(m_links, std::move(inValue));
}

void ProjectConfiguration::resolveLinksFromProject(const std::string& inProjectName, const bool inStaticLib)
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
const StringList& ProjectConfiguration::projectStaticLinks() const noexcept
{
	return m_projectStaticLinks;
}

/*****************************************************************************/
const StringList& ProjectConfiguration::staticLinks() const noexcept
{
	return m_staticLinks;
}

void ProjectConfiguration::addStaticLinks(StringList& inList)
{
	// -Wl,-Bstatic -l
	List::forEach(inList, this, &ProjectConfiguration::addStaticLink);
}

void ProjectConfiguration::addStaticLink(std::string& inValue)
{
	List::addIfDoesNotExist(m_staticLinks, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectConfiguration::libDirs() const noexcept
{
	return m_libDirs;
}

void ProjectConfiguration::addLibDirs(StringList& inList)
{
	// -L
	List::forEach(inList, this, &ProjectConfiguration::addLibDir);
}

void ProjectConfiguration::addLibDir(std::string& inValue)
{
	if (inValue.back() != '/')
		inValue += '/';

	// TODO: check other places this can be done
	parseStringVariables(inValue);
	Path::sanitize(inValue);

	List::addIfDoesNotExist(m_libDirs, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectConfiguration::includeDirs() const noexcept
{
	return m_includeDirs;
}

void ProjectConfiguration::addIncludeDirs(StringList& inList)
{
	// -I
	List::forEach(inList, this, &ProjectConfiguration::addIncludeDir);
}

void ProjectConfiguration::addIncludeDir(std::string& inValue)
{
	if (inValue.back() != '/')
		inValue += '/';

	parseStringVariables(inValue);
	Path::sanitize(inValue);

	List::addIfDoesNotExist(m_includeDirs, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectConfiguration::runDependencies() const noexcept
{
	return m_runDependencies;
}

void ProjectConfiguration::addRunDependencies(StringList& inList)
{
	List::forEach(inList, this, &ProjectConfiguration::addRunDependency);
}

void ProjectConfiguration::addRunDependency(std::string& inValue)
{
	if (inValue.back() != '/')
		inValue += '/';

	parseStringVariables(inValue);
	Path::sanitize(inValue);

	List::addIfDoesNotExist(m_runDependencies, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectConfiguration::warnings() const noexcept
{
	return m_warnings;
}

void ProjectConfiguration::addWarnings(StringList& inList)
{
	List::forEach(inList, this, &ProjectConfiguration::addWarning);
	m_warningsPreset = ProjectWarnings::Custom;
}

void ProjectConfiguration::addWarning(std::string& inValue)
{
	if (String::equals(inValue.substr(0, 2), "-W"))
	{
		Diagnostic::warn(fmt::format("Removing '-W' prefix from '{}'", inValue));
		inValue = inValue.substr(2);
	}

	List::addIfDoesNotExist(m_warnings, std::move(inValue));
}

void ProjectConfiguration::setWarningPreset(const std::string& inValue)
{
	m_warnings = parseWarnings(inValue);
}

ProjectWarnings ProjectConfiguration::warningsPreset() const noexcept
{
	return m_warningsPreset;
}

/*****************************************************************************/
bool ProjectConfiguration::warningsTreatedAsErrors() const noexcept
{
	return static_cast<int>(m_warningsPreset) >= static_cast<int>(ProjectWarnings::Error);
}

/*****************************************************************************/
bool ProjectConfiguration::includeInBuild() const noexcept
{
	return m_includeInBuild;
}

void ProjectConfiguration::setIncludeInBuild(const bool inValue)
{
	m_includeInBuild &= inValue;
}

/*****************************************************************************/
const StringList& ProjectConfiguration::compileOptions() const noexcept
{
	return m_compileOptions;
}

void ProjectConfiguration::addCompileOptions(StringList& inList)
{
	List::forEach(inList, this, &ProjectConfiguration::addCompileOption);
}

void ProjectConfiguration::addCompileOption(std::string& inValue)
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
const StringList& ProjectConfiguration::linkerOptions() const noexcept
{
	return m_linkerOptions;
}
void ProjectConfiguration::addLinkerOptions(StringList& inList)
{
	List::forEach(inList, this, &ProjectConfiguration::addLinkerOption);
}
void ProjectConfiguration::addLinkerOption(std::string& inValue)
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
const StringList& ProjectConfiguration::macosFrameworkPaths() const noexcept
{
	return m_macosFrameworkPaths;
}

void ProjectConfiguration::addMacosFrameworkPaths(StringList& inList)
{
	// -F
	List::forEach(inList, this, &ProjectConfiguration::addMacosFrameworkPath);
}

void ProjectConfiguration::addMacosFrameworkPath(std::string& inValue)
{
	if (inValue.back() != '/')
		inValue += '/';

	parseStringVariables(inValue);
	Path::sanitize(inValue);

	List::addIfDoesNotExist(m_macosFrameworkPaths, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectConfiguration::macosFrameworks() const noexcept
{
	return m_macosFrameworks;
}

void ProjectConfiguration::addMacosFrameworks(StringList& inList)
{
	// -framework *.framework
	List::forEach(inList, this, &ProjectConfiguration::addMacosFramework);
}

void ProjectConfiguration::addMacosFramework(std::string& inValue)
{
	List::addIfDoesNotExist(m_macosFrameworks, std::move(inValue));
}

/*****************************************************************************/
const std::string& ProjectConfiguration::name() const noexcept
{
	return m_name;
}

void ProjectConfiguration::setName(const std::string& inValue) noexcept
{
	m_name = inValue;
}

/*****************************************************************************/
const std::string& ProjectConfiguration::outputFile() const noexcept
{
	return m_outputFile;
}

/*****************************************************************************/
const std::string& ProjectConfiguration::cStandard() const noexcept
{
	return m_cStandard;
}

void ProjectConfiguration::setCStandard(const std::string& inValue) noexcept
{
	m_cStandard = inValue;
}

/*****************************************************************************/
const std::string& ProjectConfiguration::cppStandard() const noexcept
{
	return m_cppStandard;
}

void ProjectConfiguration::setCppStandard(const std::string& inValue) noexcept
{
	m_cppStandard = inValue;
}

/*****************************************************************************/
CodeLanguage ProjectConfiguration::language() const noexcept
{
	return m_language;
}

void ProjectConfiguration::setLanguage(const std::string& inValue) noexcept
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
		chalet_assert(false, "Invalid language for ProjectConfiguration::setLanguage");
		m_language = CodeLanguage::None;
	}
}

/*****************************************************************************/
const StringList& ProjectConfiguration::files() const noexcept
{
	return m_files;
}

void ProjectConfiguration::addFiles(StringList& inList)
{
	List::forEach(inList, this, &ProjectConfiguration::addFile);
}

void ProjectConfiguration::addFile(std::string& inValue)
{
	parseStringVariables(inValue);
	Path::sanitize(inValue);
	List::addIfDoesNotExist(m_files, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectConfiguration::locations() const noexcept
{
	return m_locations;
}

void ProjectConfiguration::addLocations(StringList& inList)
{
	List::forEach(inList, this, &ProjectConfiguration::addLocation);
}

void ProjectConfiguration::addLocation(std::string& inValue)
{
	parseStringVariables(inValue);
	Path::sanitize(inValue);

	List::addIfDoesNotExist(m_locations, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectConfiguration::locationExcludes() const noexcept
{
	return m_locationExcludes;
}

void ProjectConfiguration::addLocationExcludes(StringList& inList)
{
	List::forEach(inList, this, &ProjectConfiguration::addLocationExclude);
}

void ProjectConfiguration::addLocationExclude(std::string& inValue)
{
	parseStringVariables(inValue);
	Path::sanitize(inValue);

	List::addIfDoesNotExist(m_locationExcludes, std::move(inValue));
}

/*****************************************************************************/
const StringList& ProjectConfiguration::scripts() const noexcept
{
	return m_scripts;
}

void ProjectConfiguration::addScripts(StringList& inList)
{
	List::forEach(inList, this, &ProjectConfiguration::addScript);
}

void ProjectConfiguration::addScript(std::string& inValue)
{
	parseStringVariables(inValue);
	Path::sanitize(inValue);

	List::addIfDoesNotExist(m_scripts, std::move(inValue));
}

bool ProjectConfiguration::hasScripts() const noexcept
{
	return !m_scripts.empty();
}

/*****************************************************************************/
const std::string& ProjectConfiguration::description() const noexcept
{
	return m_description;
}

void ProjectConfiguration::setDescription(const std::string& inValue) noexcept
{
	m_description = inValue;
}

/*****************************************************************************/
const std::string& ProjectConfiguration::pch() const noexcept
{
	return m_pch;
}

void ProjectConfiguration::setPch(const std::string& inValue) noexcept
{
	m_pch = inValue;

	parseStringVariables(m_pch);
	Path::sanitize(m_pch);

	std::string path = String::getPathFolder(m_pch);
	addLocation(path);
}

bool ProjectConfiguration::usesPch() const noexcept
{
	return !m_pch.empty();
}

/*****************************************************************************/
const StringList& ProjectConfiguration::runArguments() const noexcept
{
	return m_runArguments;
}

void ProjectConfiguration::addRunArguments(StringList& inList)
{
	List::forEach(inList, this, &ProjectConfiguration::addRunArgument);
}

void ProjectConfiguration::addRunArgument(std::string& inValue)
{
	m_runArguments.push_back(std::move(inValue));
}

/*****************************************************************************/
const std::string& ProjectConfiguration::linkerScript() const noexcept
{
	return m_linkerScript;
}

void ProjectConfiguration::setLinkerScript(const std::string& inValue) noexcept
{
	m_linkerScript = inValue;
}

/*****************************************************************************/
ProjectKind ProjectConfiguration::kind() const noexcept
{
	return m_kind;
}

void ProjectConfiguration::setKind(const ProjectKind inValue) noexcept
{
	m_kind = inValue;
}

void ProjectConfiguration::setKind(const std::string& inValue)
{
	m_kind = parseProjectKind(inValue);
}

/*****************************************************************************/
bool ProjectConfiguration::cmake() const noexcept
{
	return m_cmake;
}

void ProjectConfiguration::setCmake(const bool inValue) noexcept
{
	m_cmake = inValue;
}

/*****************************************************************************/
bool ProjectConfiguration::cmakeRecheck() const noexcept
{
	return m_cmakeRecheck;
}

void ProjectConfiguration::setCmakeRecheck(const bool inValue) noexcept
{
	m_cmakeRecheck = inValue;
}

/*****************************************************************************/
const StringList& ProjectConfiguration::cmakeDefines() const noexcept
{
	return m_cmakeDefines;
}

void ProjectConfiguration::addCmakeDefines(StringList& inList)
{
	List::forEach(inList, this, &ProjectConfiguration::addCmakeDefine);
}

void ProjectConfiguration::addCmakeDefine(std::string& inValue)
{
	List::addIfDoesNotExist(m_cmakeDefines, std::move(inValue));
}

/*****************************************************************************/
bool ProjectConfiguration::dumpAssembly() const noexcept
{
	return m_dumpAssembly;
}

void ProjectConfiguration::setDumpAssembly(const bool inValue) noexcept
{
	m_dumpAssembly = inValue;
}

/*****************************************************************************/
bool ProjectConfiguration::objectiveCxx() const noexcept
{
	return m_objectiveCxx;
}
void ProjectConfiguration::setObjectiveCxx(const bool inValue) noexcept
{
	m_objectiveCxx = inValue;
}

/*****************************************************************************/
bool ProjectConfiguration::rtti() const noexcept
{
	return m_rtti;
}

void ProjectConfiguration::setRtti(const bool inValue) noexcept
{
	m_rtti = inValue;
}

/*****************************************************************************/
bool ProjectConfiguration::runProject() const noexcept
{
	return m_runProject;
}

void ProjectConfiguration::setRunProject(const bool inValue) noexcept
{
	m_runProject = inValue;
}

/*****************************************************************************/
bool ProjectConfiguration::staticLinking() const noexcept
{
	return m_staticLinking;
}

void ProjectConfiguration::setStaticLinking(const bool inValue) noexcept
{
	m_staticLinking = inValue;
}

/*****************************************************************************/
bool ProjectConfiguration::posixThreads() const noexcept
{
	return m_posixThreads;
}

void ProjectConfiguration::setPosixThreads(const bool inValue) noexcept
{
	m_posixThreads = inValue;
}

/*****************************************************************************/
bool ProjectConfiguration::windowsPrefixOutputFilename() const noexcept
{
	bool staticLib = m_kind == ProjectKind::StaticLibrary;
	return m_windowsPrefixOutputFilename || staticLib;
}

void ProjectConfiguration::setWindowsPrefixOutputFilename(const bool inValue) noexcept
{
	m_windowsPrefixOutputFilename = inValue;
}

/*****************************************************************************/
bool ProjectConfiguration::windowsOutputDef() const noexcept
{
	return m_windowsOutputDef;
}
void ProjectConfiguration::setWindowsOutputDef(const bool inValue) noexcept
{
	m_windowsOutputDef = inValue;
}

/*****************************************************************************/
void ProjectConfiguration::parseStringVariables(std::string& outString)
{
	String::replaceAll(outString, "${configuration}", m_buildConfiguration);

	const auto& externalDepDir = m_environment.externalDepDir();
	if (!externalDepDir.empty())
	{
		String::replaceAll(outString, "${externalDepDir}", externalDepDir);
	}

	if (!m_name.empty())
	{
		String::replaceAll(outString, "${name}", m_name);
	}
}

/*****************************************************************************/
ProjectKind ProjectConfiguration::parseProjectKind(const std::string& inValue)
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
void ProjectConfiguration::parseOutputFilename(const bool inWindowsMsvc) noexcept
{
	chalet_assert(!m_name.empty(), "m_name is blank");

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

#if !defined(CHALET_WIN32)
	UNUSED(inWindowsMsvc);
#endif

	if (staticLib)
	{
#if defined(CHALET_WIN32)
		if (inWindowsMsvc)
			libraryExtension = "-s.lib";
		else
#endif
			libraryExtension = "-s.a";
	}

	switch (m_kind)
	{
		case ProjectKind::ConsoleApplication:
		case ProjectKind::DesktopApplication: {
			m_outputFile = m_name + executableExtension;
			break;
		}
		case ProjectKind::SharedLibrary:
		case ProjectKind::StaticLibrary: {
			if (!windowsPrefixOutputFilename())
			{
				m_outputFile = m_name + libraryExtension;
			}
			else
			{
				m_outputFile = "lib" + m_name + libraryExtension;
			}
			break;
		}
		default:
			break;
	}
}

/*****************************************************************************/
StringList ProjectConfiguration::getDefaultCmakeDefines()
{
	// TODO: Only if using bash ... this define might not be needed at all
	return { "CMAKE_SH=\"CMAKE_SH-NOTFOUND\"" };
}

/*****************************************************************************/
// TODO: These will need numerous discussions as to how they can be categorized
//
StringList ProjectConfiguration::parseWarnings(const std::string& inValue)
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
