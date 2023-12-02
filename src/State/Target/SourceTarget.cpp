/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/SourceTarget.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{

/*****************************************************************************/
SourceTarget::SourceTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::Source),
	m_warningsPresetString("none"),
	m_inputCharset("UTF-8"),
	m_executionCharset("UTF-8")
{
}

/*****************************************************************************/
bool SourceTarget::initialize()
{
	// Timer timer;

	if (!IBuildTarget::initialize())
		return false;

	if (List::contains<std::string>(m_warnings, "error"))
		m_treatWarningsAsErrors = true;

	const auto globMessage = "Check that they exist and glob patterns can be resolved";
	if (!processEachPathList(std::move(m_appleFrameworkPaths), [this](std::string&& inValue) {
			return Files::addPathToListWithGlob(std::move(inValue), m_appleFrameworkPaths, GlobMatch::Folders);
		}))
	{
		Diagnostic::error("There was a problem resolving the macos framework paths for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}

	if (!processEachPathList(std::move(m_libDirs), [this](std::string&& inValue) {
			return Files::addPathToListWithGlob(std::move(inValue), m_libDirs, GlobMatch::Folders);
		}))
	{
		Diagnostic::error("There was a problem resolving the lib directories for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}

	if (!processEachPathList(std::move(m_includeDirs), [this](std::string&& inValue) {
			return Files::addPathToListWithGlob(std::move(inValue), m_includeDirs, GlobMatch::Folders);
		}))
	{
		Diagnostic::error("There was a problem resolving the include directories for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}

	m_headers = m_files;

	// TODO: This right here, is slow - it accounts for most of the time spent in this method
	//
	if (!processEachPathList(std::move(m_files), [this](std::string&& inValue) {
			return Files::addPathToListWithGlob(std::move(inValue), m_files, GlobMatch::Files);
		}))
	{
		Diagnostic::error("There was a problem resolving the files for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}

	// LOG("--", this->name(), timer.asString());

	if (!processEachPathList(std::move(m_fileExcludes), [this](std::string&& inValue) {
			return Files::addPathToListWithGlob(std::move(inValue), m_fileExcludes, GlobMatch::FilesAndFolders);
		}))
	{
		Diagnostic::error("There was a problem resolving the excluded files for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}

	if (!processEachPathList(std::move(m_copyFilesOnRun), [this](std::string&& inValue) {
			return Files::addPathToListWithGlob(std::move(inValue), m_copyFilesOnRun, GlobMatch::FilesAndFolders);
		}))
	{
		Diagnostic::error("There was a problem resolving the files to copy on run for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}

	if (!replaceVariablesInPathList(m_defines))
		return false;

	if (!replaceVariablesInPathList(m_configureFiles))
		return false;

	if (!m_state.replaceVariablesInString(m_precompiledHeader, this))
		return false;

	if (!replaceVariablesInPathList(m_compileOptions))
		return false;

	if (!replaceVariablesInPathList(m_linkerOptions))
		return false;

	if (!replaceVariablesInPathList(m_links))
		return false;

	if (!replaceVariablesInPathList(m_staticLinks))
		return false;

	if (!removeExcludedFiles())
		return false;

	if (!determinePicType())
		return false;

	if (m_unityBuild && !initializeUnityBuild())
		return false;

	if (m_metadata != nullptr)
	{
		if (!m_metadata->initialize(m_state, this))
			return false;
	}

	auto& config = m_state.configuration;
	if (config.sanitizeUndefinedBehavior())
	{
		if (m_state.environment->isWindowsClang() && !m_staticRuntimeLibrary)
		{
			Diagnostic::warn("'staticRuntimeLibrary' was enabled in order to use the Undefined Behavior sanitizer.");
			m_staticRuntimeLibrary = true;
		}
	}

	return true;
}

/*****************************************************************************/
bool SourceTarget::removeExcludedFiles()
{
	if (!m_fileExcludes.empty())
	{
		std::string excludes = String::join(m_fileExcludes);

		auto itr = m_files.begin();
		while (itr != m_files.end())
		{
			if (itr->empty())
			{
				itr = m_files.erase(itr);
			}
			else
			{
				auto& path = *itr;
				bool excluded = String::contains(path, excludes);

				if (!excluded)
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
		}
	}

	return true;
}

/*****************************************************************************/
bool SourceTarget::determinePicType()
{
	if (m_picType == PositionIndependentCodeType::Auto)
	{
		if (m_kind == SourceKind::Executable)
		{
			m_picType = PositionIndependentCodeType::Executable;
		}
		else if (m_kind == SourceKind::SharedLibrary)
		{
			m_picType = PositionIndependentCodeType::Shared;
		}
		else if (m_kind == SourceKind::StaticLibrary)
		{
			const auto& targetName = this->name();
			for (const auto& target : m_state.targets)
			{
				if (String::equals(targetName, target->name()))
					continue;

				if (target->isSources())
				{
					const auto& sources = static_cast<const SourceTarget&>(*target);
					const auto& links = sources.projectStaticLinks();
					if (List::contains(links, targetName))
					{
						if (sources.isExecutable())
							m_picType = PositionIndependentCodeType::Executable;
						else if (sources.isSharedLibrary())
							m_picType = PositionIndependentCodeType::Shared;

						break;
					}
				}
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool SourceTarget::initializeUnityBuild()
{
	bool excludeInStrategies = m_state.toolchain.strategy() == StrategyType::MSBuild;
	if (m_unityBuild && !excludeInStrategies)
	{
		for (auto& file : m_files)
		{
			// this is a hack to make sure the common file ext gets set
			// If it's not, the source build hash changes
			m_state.paths.getSourceType(file);
		}

		if (m_files.empty())
		{
			Diagnostic::error("Unity build requested in a project with no 'files'.");
			return false;
		}

		m_unityBuildContents = "// Unity build file generated by Chalet\n\n";

		for (std::string file : m_files)
		{
			for (auto& include : m_includeDirs)
			{
				if (String::startsWith(include, file))
				{
					auto size = include.size();
					if (file[size] == '/')
						file = file.substr(size + 1);
					else
						file = file.substr(size);

					break;
				}
			}
			m_unityBuildContents += fmt::format("#include \"{}\"\n", file);
		}

		std::string sourceFile;
		if (!generateUnityBuildFile(sourceFile))
			return false;

		m_files = { std::move(sourceFile) };
	}

	return true;
}

/*****************************************************************************/
std::string SourceTarget::getUnityBuildFile() const
{
	chalet_assert(!m_unityBuildContents.empty(), "unity build was not initialized before the build file was generated.");
	return m_state.paths.getUnityBuildSourceFilename(*this);
}

/*****************************************************************************/
bool SourceTarget::generateUnityBuildFile(std::string& sourceFile) const
{
	chalet_assert(!m_unityBuildContents.empty(), "unity build was not initialized before the build file was generated.");

	sourceFile = getUnityBuildFile();
	if (sourceFile.empty())
		return false;

	auto folder = String::getPathFolder(sourceFile);
	if (!Files::pathExists(folder))
	{
		if (!Files::makeDirectory(folder))
		{
			Diagnostic::error("Error creating directory: '{}'", folder);
			return false;
		}
	}

	bool generateFile = true;
	std::string existingContents;
	if (Files::pathExists(sourceFile))
	{
		existingContents = Files::getFileContents(sourceFile);
		if (!existingContents.empty())
			existingContents.pop_back(); // last '\n'

		generateFile = existingContents.empty() || !String::equals(m_unityBuildContents, existingContents);
	}

	if (generateFile)
	{
		if (!Files::createFileWithContents(sourceFile, m_unityBuildContents))
		{
			Diagnostic::error("Error creating file: '{}'", sourceFile);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool SourceTarget::validate()
{
	chalet_assert(m_kind != SourceKind::None, "SourceTarget msut be executable, sharedLibrary or staticLibrary");
	chalet_assert(m_picType != PositionIndependentCodeType::Auto, "SourceTarget picType was not initialized");

	bool result = true;

	if (m_kind == SourceKind::None)
	{
		Diagnostic::error("A valid 'kind' was not found.");
		result = false;
	}

	if (m_files.empty())
	{
		const auto& targetName = this->name();
		Diagnostic::error("Either no 'files' were specified, or their resolved path(s) in do not exist. Check to make sure they are correct.", targetName);
		result = false;
	}

	if (!m_precompiledHeader.empty())
	{
		auto pch = getPrecompiledHeaderResolvedToRoot();
		if (!Files::pathExists(pch))
		{
			Diagnostic::error("Precompiled header '{}' was not found.", pch);
			result = false;
		}

		auto rootPath = String::getPathFolder(m_precompiledHeader);
		if (rootPath.empty() || String::startsWith("..", rootPath))
		{
			Diagnostic::error("Precompiled header '{}' must be placed in a child directory (such as 'src').", m_precompiledHeader);
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
const std::string& SourceTarget::getHash() const
{
	if (m_hash.empty())
	{
		auto files = String::join(m_files);
		auto defines = String::join(m_defines);
		auto links = String::join(m_links);
		auto staticLinks = String::join(m_staticLinks);
		auto warnings = String::join(m_warnings);
		auto compileOptions = String::join(m_compileOptions);
		auto libDirs = String::join(m_libDirs);
		auto includeDirs = String::join(m_includeDirs);
		auto appleFrameworkPaths = String::join(m_appleFrameworkPaths);
		auto appleFrameworks = String::join(m_appleFrameworks);
		auto configureFiles = String::join(m_configureFiles);

		auto hashable = Hash::getHashableString(this->name(), files, defines, links, staticLinks, warnings, compileOptions, libDirs, includeDirs, appleFrameworkPaths, appleFrameworks, configureFiles, m_warningsPresetString, m_cStandard, m_cppStandard, m_precompiledHeader, m_inputCharset, m_executionCharset, m_windowsApplicationManifest, m_windowsApplicationIcon, m_buildSuffix, m_threads, m_cppFilesystem, m_cppModules, m_cppConcepts, m_runtimeTypeInformation, m_exceptions, m_fastMath, m_staticRuntimeLibrary, m_treatWarningsAsErrors, m_posixThreads, m_invalidWarningPreset, m_unityBuild, m_windowsApplicationManifestGenerationEnabled, m_mingwUnixSharedLibraryNamingConvention, m_setWindowsPrefixOutputFilename, m_windowsOutputDef, m_kind, m_language, m_warningsPreset, m_windowsSubSystem, m_windowsEntryPoint, m_picType);

		m_hash = Hash::string(hashable);
	}

	return m_hash;
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
void SourceTarget::setMetadata(Ref<TargetMetadata>&& inValue)
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
			if (project.isStaticLibrary())
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
			else if (project.isSharedLibrary())
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
const StringList& SourceTarget::appleFrameworkPaths() const noexcept
{
	return m_appleFrameworkPaths;
}

void SourceTarget::addAppleFrameworkPaths(StringList&& inList)
{
	// -F
	List::forEach(inList, this, &SourceTarget::addAppleFrameworkPath);
}

void SourceTarget::addAppleFrameworkPath(std::string&& inValue)
{
	List::addIfDoesNotExist(m_appleFrameworkPaths, inValue);
}

/*****************************************************************************/
const StringList& SourceTarget::appleFrameworks() const noexcept
{
	return m_appleFrameworks;
}

void SourceTarget::addAppleFrameworks(StringList&& inList)
{
	// -framework *.framework
	List::forEach(inList, this, &SourceTarget::addAppleFramework);
}

void SourceTarget::addAppleFramework(std::string&& inValue)
{
	List::addIfDoesNotExist(m_appleFrameworks, std::move(inValue));
}

/*****************************************************************************/
const StringList& SourceTarget::copyFilesOnRun() const noexcept
{
	return m_copyFilesOnRun;
}

void SourceTarget::addCopyFilesOnRun(StringList&& inList)
{
	List::forEach(inList, this, &SourceTarget::addCopyFileOnRun);
}

void SourceTarget::addCopyFileOnRun(std::string&& inValue)
{
	// if (inValue.back() != '/')
	// 	inValue += '/'; // no!

	List::addIfDoesNotExist(m_copyFilesOnRun, std::move(inValue));
}

/*****************************************************************************/
const std::string& SourceTarget::outputFile() const noexcept
{
	return m_outputFile;
}

/*****************************************************************************/
std::string SourceTarget::getOutputFileWithoutPrefix() const noexcept
{
	if (isExecutable())
	{
		return name() + m_state.environment->getExecutableExtension();
	}
	else if (isSharedLibrary())
	{
		return name() + m_state.environment->getSharedLibraryExtension();
	}
	else if (isStaticLibrary())
	{
		return name() + m_state.environment->getArchiveExtension();
	}

	chalet_assert(false, "getOutputFileWithoutPrefix() returned empty string");
	return std::string();
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
		m_language = CodeLanguage::ObjectiveCPlusPlus;
	}
	else if (String::equals("Objective-C", inValue))
	{
		m_language = CodeLanguage::ObjectiveC;
	}
	else
	{
		chalet_assert(false, "Invalid language for SourceTarget::setLanguage");
		m_language = CodeLanguage::None;
	}
}

/*****************************************************************************/
SourceType SourceTarget::getDefaultSourceType() const
{
	switch (m_language)
	{
		case CodeLanguage::ObjectiveC: return SourceType::ObjectiveC;
		case CodeLanguage::ObjectiveCPlusPlus: return SourceType::ObjectiveCPlusPlus;
		case CodeLanguage::C: return SourceType::C;
		case CodeLanguage::CPlusPlus: return SourceType::CPlusPlus;
		default: break;
	}

	return SourceType::Unknown;
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
	// TODO: This would auto-include the root of the glob pattern, but it might be confusing to people
	/*auto lastStar = inValue.find_first_of('*');
	if (lastStar != std::string::npos)
	{
		auto end = inValue.find_last_of('/', lastStar);
		if (end != std::string::npos)
		{
			std::string basePath = inValue.substr(0, end);
			if (!basePath.empty())
			{
				addIncludeDir(std::move(basePath));
			}
		}
	}*/

	List::addIfDoesNotExist(m_files, std::move(inValue));
}

/*****************************************************************************/
StringList SourceTarget::getHeaderFiles() const
{
	// Used as a last resort (right now, in project export)

	StringList headers = m_headers;
	if (!processEachPathList(std::move(headers), [&headers](std::string&& inValue) {
			auto header = String::getPathFolderBaseName(inValue);
			header += ".{h,hh,hpp,hxx,H,inl,i,ii,ixx,ipp,txx,tpp,tpl,h\\+\\+}";
			return Files::addPathToListWithGlob(std::move(header), headers, GlobMatch::Files);
		}))
		return StringList{};

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
	return !m_precompiledHeader.empty();
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
const std::string& SourceTarget::buildSuffix() const noexcept
{
	return !m_buildSuffix.empty() ? m_buildSuffix : this->name();
}

void SourceTarget::setBuildSuffix(std::string&& inValue) noexcept
{
	m_buildSuffix = std::move(inValue);
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
bool SourceTarget::positionIndependentCode() const noexcept
{
	return m_picType == PositionIndependentCodeType::Shared;
}

bool SourceTarget::positionIndependentExecutable() const noexcept
{
	return m_picType == PositionIndependentCodeType::Executable;
}

void SourceTarget::setPicType(const bool inValue) noexcept
{
	m_picType = inValue ? PositionIndependentCodeType::Auto : PositionIndependentCodeType::None;
}

void SourceTarget::setPicType(const std::string& inValue)
{
	if (String::equals("shared", inValue))
	{
		m_picType = PositionIndependentCodeType::Shared;
	}
	else if (String::equals("shared", inValue))
	{
		m_picType = PositionIndependentCodeType::Executable;
	}
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
	return m_language == CodeLanguage::ObjectiveC || m_language == CodeLanguage::ObjectiveCPlusPlus;
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
bool SourceTarget::unityBuild() const noexcept
{
	return m_unityBuild;
}
void SourceTarget::setUnityBuild(const bool inValue) noexcept
{
	m_unityBuild = inValue;
}

/*****************************************************************************/
void SourceTarget::setMinGWUnixSharedLibraryNamingConvention(const bool inValue) noexcept
{
	m_mingwUnixSharedLibraryNamingConvention = inValue;
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
std::string SourceTarget::getPrecompiledHeaderResolvedToRoot() const
{
	auto& root = m_state.paths.rootDirectory();
	if (!root.empty())
		return fmt::format("{}/{}", root, m_precompiledHeader);

	return m_precompiledHeader;
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
	if (isExecutable())
	{
		m_outputFile = getOutputFileWithoutPrefix();
	}
	else
	{
		auto libraryPrefix = m_state.environment->getLibraryPrefix(m_mingwUnixSharedLibraryNamingConvention);
		m_outputFile = libraryPrefix + getOutputFileWithoutPrefix();
	}
}

/*****************************************************************************/
StringList SourceTarget::getResolvedRunDependenciesList() const
{
	StringList ret;

	for (auto& dep : m_copyFilesOnRun)
	{
		if (Files::pathExists(dep))
		{
			ret.push_back(dep);
			continue;
		}

		std::string resolved;
		if (isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*this);
			const auto& compilerPathBin = m_state.toolchain.compilerCxx(project.language()).binDir;

			resolved = fmt::format("{}/{}", compilerPathBin, dep);
			if (Files::pathExists(resolved))
			{
				ret.emplace_back(std::move(resolved));
				continue;
			}
		}

		bool found = false;
		for (auto& path : m_state.workspace.searchPaths())
		{
			resolved = fmt::format("{}/{}", path, dep);
			if (Files::pathExists(resolved))
			{
				ret.emplace_back(std::move(resolved));
				found = true;
				break;
			}
		}

		if (!found)
		{
			resolved = Files::which(dep);
			if (!resolved.empty())
			{
				ret.emplace_back(std::move(resolved));
			}
		}
	}

	return ret;
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
}
