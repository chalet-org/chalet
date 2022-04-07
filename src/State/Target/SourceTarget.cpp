/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/SourceTarget.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
SourceTarget::SourceTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::Project),
	m_warningsPresetString("none"),
	m_inputCharset("UTF-8"),
	m_executionCharset("UTF-8")
{
}

/*****************************************************************************/
bool SourceTarget::initialize()
{
	if (!IBuildTarget::initialize())
		return false;

	if (!m_warnings.empty())
		m_warningsPreset = ProjectWarningPresets::Custom;

	if (List::contains<std::string>(m_warnings, "error"))
		m_treatWarningsAsErrors = true;

	processEachPathList(std::move(m_macosFrameworkPaths), [this](std::string&& inValue) {
		Commands::addPathToListWithGlob(std::move(inValue), m_macosFrameworkPaths, GlobMatch::Folders);
	});

	processEachPathList(std::move(m_libDirs), [this](std::string&& inValue) {
		Commands::addPathToListWithGlob(std::move(inValue), m_libDirs, GlobMatch::Folders);
	});

	processEachPathList(std::move(m_includeDirs), [this](std::string&& inValue) {
		Commands::addPathToListWithGlob(std::move(inValue), m_includeDirs, GlobMatch::Folders);
	});

	m_headers = m_files;
	processEachPathList(std::move(m_files), [this](std::string&& inValue) {
		Commands::addPathToListWithGlob(std::move(inValue), m_files, GlobMatch::Files);
	});
	processEachPathList(std::move(m_fileExcludes), [this](std::string&& inValue) {
		Commands::addPathToListWithGlob(std::move(inValue), m_fileExcludes, GlobMatch::FilesAndFolders);
	});

	replaceVariablesInPathList(m_defines);
	replaceVariablesInPathList(m_configureFiles);

	const auto& targetName = this->name();
	m_state.replaceVariablesInPath(m_precompiledHeader, targetName);

	if (!m_compileOptionsRaw.empty())
	{
		m_state.replaceVariablesInPath(m_compileOptionsRaw, targetName);
		m_compileOptions = parseCommandLineOptions(m_compileOptionsRaw);
	}

	if (!m_linkerOptionsRaw.empty())
	{
		m_state.replaceVariablesInPath(m_linkerOptionsRaw, targetName);
		m_linkerOptions = parseCommandLineOptions(m_linkerOptionsRaw);
	}

	if (!m_fileExcludes.empty())
	{
		std::string excludes = String::join(m_fileExcludes);

		auto itr = m_files.begin();
		while (itr != m_files.end())
		{
			if (!itr->empty())
			{
				auto& path = *itr;
				bool excluded = false;

				if (!String::contains(path, excludes))
				{
					for (auto& exclude : m_fileExcludes)
					{
						if (String::contains(exclude, path))
						{
							excluded = true;
							break;
						}
					}
				}

				if (excluded)
					itr = m_files.erase(itr);
				else
					++itr;
			}
			else
			{
				// no dependencies - we don't care about it
				itr = m_files.erase(itr);
			}
		}
	}

	if (m_metadata != nullptr)
	{
		if (!m_metadata->initialize())
			return false;
	}

	return true;
}

/*****************************************************************************/
bool SourceTarget::validate()
{
	chalet_assert(m_kind != SourceKind::None, "SourceTarget msut be executable, sharedLibrary or staticLibrary");

	const auto& targetName = this->name();
	bool result = true;

	if (m_kind == SourceKind::None)
	{
		Diagnostic::error("A valid 'kind' was not found.");
		result = false;
	}

	if (m_files.empty())
	{
		Diagnostic::error("No 'files' were specified, but are required.", targetName);
		result = false;
	}

	if (!m_precompiledHeader.empty())
	{
		const auto& pch = !m_state.paths.rootDirectory().empty() ? fmt::format("{}/{}", m_state.paths.rootDirectory(), m_precompiledHeader) : m_precompiledHeader;
		if (!Commands::pathExists(pch))
		{
			Diagnostic::error("Precompiled header '{}' was not found.", pch);
			result = false;
		}
	}

	if (m_configureFiles.size() > 1)
	{
		StringList tmpList;
		for (const auto& configureFile : m_configureFiles)
		{
			auto file = String::getPathFilename(configureFile);
			if (!List::contains(tmpList, file))
			{
				tmpList.push_back(std::move(file));
			}
			else
			{
				Diagnostic::error("Configure files in the same source target must have unique names. Found more than one: {}", file);
				result = false;
			}
		}
	}

	/*for (auto& option : m_compileOptions)
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
	}*/

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
	return m_kind == SourceKind::Executable;
}

bool SourceTarget::isSharedLibrary() const noexcept
{
	return m_kind == SourceKind::SharedLibrary;
}

bool SourceTarget::isStaticLibrary() const noexcept
{
	return m_kind == SourceKind::StaticLibrary;
}

/*****************************************************************************/
bool SourceTarget::hasMetadata() const noexcept
{
	return m_metadata != nullptr;
}
const TargetMetadata& SourceTarget::metadata() const noexcept
{
	chalet_assert(m_metadata != nullptr, "metadata() accessed w/o data");
	return *m_metadata;
}
void SourceTarget::setMetadata(Shared<TargetMetadata>&& inValue)
{
	m_metadata = std::move(inValue);
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

bool SourceTarget::resolveLinksFromProject(const std::vector<BuildTarget>& inTargets, const std::string& inInputFile)
{
	for (auto& target : inTargets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<SourceTarget&>(*target);
			const auto& projectName = project.name();
			if (project.kind() == SourceKind::StaticLibrary)
			{
				for (auto& link : m_links)
				{
					if (!String::equals(projectName, link))
						continue;

					Diagnostic::error("{}: Static library target '{}' found in links for target '{}' (move to 'staticLinks')", inInputFile, projectName, this->name());
					return false;
				}

				for (auto& link : m_staticLinks)
				{
					if (!String::equals(projectName, link))
						continue;

					List::addIfDoesNotExist(m_projectStaticLinks, std::string(link));
				}
			}
			else if (project.kind() == SourceKind::SharedLibrary)
			{
				for (auto& link : m_links)
				{
					if (!String::equals(projectName, link))
						continue;

					List::addIfDoesNotExist(m_projectSharedLinks, std::string(link));
				}
			}
		}
	}

	return true;
}

/*****************************************************************************/
const StringList& SourceTarget::projectStaticLinks() const noexcept
{
	return m_projectStaticLinks;
}

const StringList& SourceTarget::projectSharedLinks() const noexcept
{
	return m_projectSharedLinks;
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

	List::addIfDoesNotExist(m_libDirs, std::move(inValue));
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

	List::addIfDoesNotExist(m_includeDirs, std::move(inValue));
}

/*****************************************************************************/
const StringList& SourceTarget::warnings() const noexcept
{
	return m_warnings;
}

void SourceTarget::addWarnings(StringList&& inList)
{
	List::forEach(inList, this, &SourceTarget::addWarning);
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
	m_warningsPreset = parseWarnings(m_warningsPresetString);
}

ProjectWarningPresets SourceTarget::warningsPreset() const noexcept
{
	return m_warningsPreset;
}

/*****************************************************************************/
const StringList& SourceTarget::compileOptions() const noexcept
{
	return m_compileOptions;
}

void SourceTarget::addCompileOptions(std::string&& inValue)
{
	m_compileOptionsRaw = std::move(inValue);
}

/*****************************************************************************/
const StringList& SourceTarget::linkerOptions() const noexcept
{
	return m_linkerOptions;
}
void SourceTarget::addLinkerOptions(std::string&& inValue)
{
	m_linkerOptionsRaw = std::move(inValue);
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
		m_cxxSpecialization = CxxSpecialization::CPlusPlus;
	}
	else if (String::equals("C", inValue))
	{
		m_language = CodeLanguage::C;
		m_cxxSpecialization = CxxSpecialization::C;
	}
	else if (String::equals("Objective-C++", inValue))
	{
		m_language = CodeLanguage::CPlusPlus;
		m_cxxSpecialization = CxxSpecialization::ObjectiveCPlusPlus;
		setObjectiveCxx(true);
	}
	else if (String::equals("Objective-C", inValue))
	{
		m_language = CodeLanguage::C;
		m_cxxSpecialization = CxxSpecialization::ObjectiveC;
		setObjectiveCxx(true);
	}
	else
	{
		chalet_assert(false, "Invalid language for SourceTarget::setLanguage");
		m_language = CodeLanguage::None;
	}
}

/*****************************************************************************/
CxxSpecialization SourceTarget::cxxSpecialization() const noexcept
{
	return m_cxxSpecialization;
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
	List::addIfDoesNotExist(m_files, std::move(inValue));
}

/*****************************************************************************/
StringList SourceTarget::getHeaderFiles() const
{
	// Used as a last resort (right now, in project export)

	StringList headers = m_headers;
	processEachPathList(std::move(headers), [&headers](std::string&& inValue) {
		auto header = String::getPathFolderBaseName(inValue);
		header += ".{h,hh,hpp,hxx,H,inl,i,ii,ixx,ipp,txx,tpp,tpl,h\\+\\+}";
		Commands::addPathToListWithGlob(std::move(header), headers, GlobMatch::Files);
	});

	return headers;
}

/*****************************************************************************/
const StringList& SourceTarget::fileExcludes() const noexcept
{
	return m_fileExcludes;
}

void SourceTarget::addFileExcludes(StringList&& inList)
{
	List::forEach(inList, this, &SourceTarget::addFileExclude);
}

void SourceTarget::addFileExclude(std::string&& inValue)
{
	List::addIfDoesNotExist(m_fileExcludes, std::move(inValue));
}

/*****************************************************************************/
const StringList& SourceTarget::configureFiles() const noexcept
{
	return m_configureFiles;
}

void SourceTarget::addConfigureFiles(StringList&& inList)
{
	List::forEach(inList, this, &SourceTarget::addConfigureFile);
}

void SourceTarget::addConfigureFile(std::string&& inValue)
{
	List::addIfDoesNotExist(m_configureFiles, std::move(inValue));
}

/*****************************************************************************/
const std::string& SourceTarget::precompiledHeader() const noexcept
{
	return m_precompiledHeader;
}

void SourceTarget::setPrecompiledHeader(std::string&& inValue) noexcept
{
	m_precompiledHeader = std::move(inValue);
}

bool SourceTarget::usesPrecompiledHeader() const noexcept
{
	// Note: Objective-C++ itself doesn't use them, but the target could be mixed
	//   with plain C++, which does use them. might be worth reworking some of this logic
	//
	bool validSpecialization = m_cxxSpecialization == CxxSpecialization::CPlusPlus
		|| m_cxxSpecialization == CxxSpecialization::ObjectiveCPlusPlus
		|| m_cxxSpecialization == CxxSpecialization::C;

	return !m_precompiledHeader.empty() && validSpecialization;
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
const std::string& SourceTarget::inputCharset() const noexcept
{
	return m_inputCharset;
}

void SourceTarget::setInputCharset(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_inputCharset = std::move(inValue);
}

/*****************************************************************************/
const std::string& SourceTarget::executionCharset() const noexcept
{
	return m_executionCharset;
}

void SourceTarget::setExecutionCharset(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_executionCharset = std::move(inValue);
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
SourceKind SourceTarget::kind() const noexcept
{
	return m_kind;
}

void SourceTarget::setKind(const SourceKind inValue) noexcept
{
	m_kind = inValue;
}

void SourceTarget::setKind(const std::string& inValue)
{
	m_kind = parseProjectKind(inValue);
}

/*****************************************************************************/
bool SourceTarget::threads() const noexcept
{
	return m_threads;
}
void SourceTarget::setThreads(const bool inValue) noexcept
{
	m_threads = inValue;
}

/*****************************************************************************/
bool SourceTarget::treatWarningsAsErrors() const noexcept
{
	return m_treatWarningsAsErrors;
}
void SourceTarget::setTreatWarningsAsErrors(const bool inValue) noexcept
{
	m_treatWarningsAsErrors = inValue;
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
bool SourceTarget::cppFilesystem() const noexcept
{
	return m_cppFilesystem;
}
void SourceTarget::setCppFilesystem(const bool inValue) noexcept
{
	m_cppFilesystem = inValue;
}

/*****************************************************************************/
bool SourceTarget::cppModules() const noexcept
{
	return m_cppModules;
}
void SourceTarget::setCppModules(const bool inValue) noexcept
{
	m_cppModules = inValue;
}

/*****************************************************************************/
bool SourceTarget::cppCoroutines() const noexcept
{
	return m_cppCoroutines;
}

void SourceTarget::setCppCoroutines(const bool inValue) noexcept
{
	m_cppCoroutines = inValue;
}

/*****************************************************************************/
bool SourceTarget::cppConcepts() const noexcept
{
	return m_cppConcepts;
}

void SourceTarget::setCppConcepts(const bool inValue) noexcept
{
	m_cppConcepts = inValue;
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
bool SourceTarget::runtimeTypeInformation() const noexcept
{
	return m_runtimeTypeInformation;
}

void SourceTarget::setRuntimeTypeInformation(const bool inValue) noexcept
{
	m_runtimeTypeInformation = inValue;
}

/*****************************************************************************/
bool SourceTarget::exceptions() const noexcept
{
	return m_exceptions;
}
void SourceTarget::setExceptions(const bool inValue) noexcept
{
	m_exceptions = inValue;
}

/*****************************************************************************/
bool SourceTarget::fastMath() const noexcept
{
	return m_fastMath;
}
void SourceTarget::setFastMath(const bool inValue) noexcept
{
	m_fastMath = inValue;
}

/*****************************************************************************/
bool SourceTarget::staticRuntimeLibrary() const noexcept
{
	return m_staticRuntimeLibrary;
}

void SourceTarget::setStaticRuntimeLibrary(const bool inValue) noexcept
{
	m_staticRuntimeLibrary = inValue;
}

/*****************************************************************************/
void SourceTarget::setMinGWUnixSharedLibraryNamingConvention(const bool inValue) noexcept
{
	m_mingwUnixSharedLibraryNamingConvention = inValue;
}

bool SourceTarget::unixSharedLibraryNamingConvention() const noexcept
{
	bool mingw = m_state.environment->isMingw();
	bool mingwWithPrefix = m_mingwUnixSharedLibraryNamingConvention && mingw;
	bool nonWindows = !mingw && !m_state.environment->isMsvc() && !m_state.environment->isWindowsClang();

	return mingwWithPrefix || nonWindows;
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
SourceKind SourceTarget::parseProjectKind(const std::string& inValue)
{
	if (String::equals("executable", inValue))
		return SourceKind::Executable;

	if (String::equals("staticLibrary", inValue))
		return SourceKind::StaticLibrary;

	if (String::equals("sharedLibrary", inValue))
		return SourceKind::SharedLibrary;

	return SourceKind::None;
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
void SourceTarget::parseOutputFilename() noexcept
{
	const auto& projectName = name();
	chalet_assert(!projectName.empty(), "parseOutputFilename: name is blank");

	bool staticLib = m_kind == SourceKind::StaticLibrary;

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
	if (executableExtension.empty() && (m_state.environment->isMingw() || m_state.environment->isWindowsTarget()))
	{
		executableExtension = ".exe";
		libraryExtension = ".dll";
	}

	if (staticLib)
	{
		if (m_state.environment->isMsvc() || m_state.environment->isWindowsClang())
			libraryExtension = ".lib";
		else
			libraryExtension = ".a";
	}

	switch (m_kind)
	{
		case SourceKind::Executable: {
			m_outputFile = projectName + executableExtension;
			m_outputFileNoPrefix = m_outputFile;
			break;
		}
		case SourceKind::SharedLibrary:
		case SourceKind::StaticLibrary: {
			if (unixSharedLibraryNamingConvention())
			{
				// TODO: version number

				m_outputFileNoPrefix = projectName + libraryExtension;
				m_outputFile = "lib" + m_outputFileNoPrefix;
			}
			else
			{
				m_outputFile = projectName + libraryExtension;
				m_outputFileNoPrefix = m_outputFile;
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
ProjectWarningPresets SourceTarget::parseWarnings(const std::string& inValue)
{
	if (String::equals("none", inValue))
		return ProjectWarningPresets::None;

	if (String::equals("minimal", inValue))
		return ProjectWarningPresets::Minimal;

	if (String::equals("extra", inValue))
		return ProjectWarningPresets::Extra;

	if (String::equals("pedantic", inValue))
		return ProjectWarningPresets::Pedantic;

	if (String::equals("strict", inValue))
		return ProjectWarningPresets::Strict;

	if (String::equals("strictPedantic", inValue))
		return ProjectWarningPresets::StrictPedantic;

	if (String::equals("veryStrict", inValue))
		return ProjectWarningPresets::VeryStrict;

	// More?
	// can't be ignored in GCC 10.2.0, so best not to use it at all
	// ret.emplace_back("switch-default");

	m_invalidWarningPreset = true;

	return ProjectWarningPresets::None;
}

/*****************************************************************************/
// Takes a string of command line options and returns a StringList
StringList SourceTarget::parseCommandLineOptions(std::string inString) const
{
	StringList ret;
	if (inString.empty())
		return ret;

	char separator = ' ';
	char quote = '"';

	std::string sub;
	std::size_t itrQuote = 0;
	std::size_t itr = 0;
	std::size_t nextNonChar = 0;
	while (itr != std::string::npos)
	{
		itr = inString.find(separator);

		sub = inString.substr(0, itr);
		itrQuote = sub.find_first_of(quote);

		if (itrQuote != std::string::npos)
		{
			itr = inString.find_first_of(quote, itrQuote + 1);
			if (itr == std::string::npos)
				break; // bad string - no closing quote

			++itr;
			sub = inString.substr(0, itr);
		}
		nextNonChar = inString.find_first_not_of(separator, itr);

		const bool nonCharFound = nextNonChar != std::string::npos;
		inString = inString.substr(nonCharFound ? nextNonChar : itr + 1);
		if (nonCharFound)
			itr = nextNonChar;

		if (!sub.empty())
		{
			while (sub.back() == separator)
				sub.pop_back();
		}

		if (!sub.empty())
			ret.push_back(sub);
	}

	return ret;
}
}
