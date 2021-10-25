/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildPaths.hpp"

#include "Compile/CompilerConfigController.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
BuildPaths::BuildPaths(const CommandLineInputs& inInputs, const BuildState& inState) :
	m_inputs(inInputs),
	m_state(inState),
	m_cExts({ "c" }),
	m_cppExts({ "cpp", "cc", "cxx", "c++", "C", "CPP", "CC", "CXX", "C++" }),
	m_resourceExts({ "rc", "RC" }),
	m_objectiveCExts({ "m", "M" }),
	m_objectiveCppExts({ "mm" })
{
}

/*****************************************************************************/
bool BuildPaths::initialize()
{
	chalet_assert(!m_initialized, "BuildPaths::initialize called twice.");

	const auto& outputDirectory = m_inputs.outputDirectory();
	if (!Commands::pathExists(outputDirectory))
	{
		Commands::makeDirectory(outputDirectory);
	}

	auto arch = m_inputs.getArchWithOptionsAsString(m_state.info.targetArchitectureTriple());
	const auto& buildConfig = m_state.info.buildConfiguration();
	const auto& toolchainPreference = m_inputs.toolchainPreferenceName();

	auto style = m_state.toolchain.buildPathStyle();
	if (style == BuildPathStyle::ToolchainName && !toolchainPreference.empty())
	{
		m_buildOutputDir = fmt::format("{}/{}_{}", outputDirectory, toolchainPreference, buildConfig);
	}
	else if (style == BuildPathStyle::Configuration)
	{
		m_buildOutputDir = fmt::format("{}/{}", outputDirectory, buildConfig);
	}
	else if (style == BuildPathStyle::ArchConfiguration)
	{
		arch = m_state.info.targetArchitectureString();
		m_buildOutputDir = fmt::format("{}/{}-{}", outputDirectory, arch, buildConfig);
	}
	else
	{
		m_buildOutputDir = fmt::format("{}/{}_{}", outputDirectory, arch, buildConfig);
	}

	m_intermediateDir = fmt::format("{}/intermediate", outputDirectory);

	{
		auto search = m_intermediateDir.find_first_not_of("./"); // if it's relative
		m_intermediateDir = m_intermediateDir.substr(search);
	}

	m_initialized = true;

	return true;
}

/*****************************************************************************/
void BuildPaths::populateFileList(const SourceTarget& inProject)
{
	if (m_fileList.find(inProject.name()) != m_fileList.end())
		return;

	// setBuildDirectoriesBasedOnProjectKind(inProject);

	SourceGroup files = getFiles(inProject);

	for (auto& file : files.list)
	{
		auto ext = String::getPathSuffix(file);
		List::addIfDoesNotExist(m_allFileExtensions, std::move(ext));
	}

	std::sort(m_allFileExtensions.begin(), m_allFileExtensions.end());

	m_fileList.emplace(inProject.name(), std::make_unique<SourceGroup>(std::move(files)));
}

/*****************************************************************************/
const std::string& BuildPaths::homeDirectory() const noexcept
{
	return m_inputs.homeDirectory();
}

/*****************************************************************************/
const std::string& BuildPaths::rootDirectory() const noexcept
{
	return m_inputs.rootDirectory();
}

/*****************************************************************************/
const std::string& BuildPaths::outputDirectory() const noexcept
{
	return m_inputs.outputDirectory();
}

const std::string& BuildPaths::buildOutputDir() const noexcept
{
	chalet_assert(!m_buildOutputDir.empty(), "BuildPaths::buildOutputDir() called before BuildPaths::initialize().");
	return m_buildOutputDir;
}

const std::string& BuildPaths::objDir() const noexcept
{
	chalet_assert(!m_objDir.empty(), "BuildPaths::objDir() called before BuildPaths::initialize().");
	return m_objDir;
}

const std::string& BuildPaths::depDir() const noexcept
{
	chalet_assert(!m_depDir.empty(), "BuildPaths::depDir() called before BuildPaths::initialize().");
	return m_depDir;
}

const std::string& BuildPaths::asmDir() const noexcept
{
	chalet_assert(!m_asmDir.empty(), "BuildPaths::asmDir() called before BuildPaths::initialize().");
	return m_asmDir;
}

const std::string& BuildPaths::intermediateDir() const noexcept
{
	chalet_assert(!m_intermediateDir.empty(), "BuildPaths::intermediateDir() called before BuildPaths::initialize().");
	return m_intermediateDir;
}

/*****************************************************************************/
const StringList& BuildPaths::allFileExtensions() const noexcept
{
	return m_allFileExtensions;
}
const StringList& BuildPaths::cExtensions() const noexcept
{
	return m_cExts;
}
const StringList& BuildPaths::cppExtensions() const noexcept
{
	return m_cppExts;
}
StringList BuildPaths::cxxExtensions() const noexcept
{
	return List::combine(m_cExts, m_cppExts);
}
const StringList& BuildPaths::objectiveCExtensions() const noexcept
{
	return m_objectiveCExts;
}
const StringList& BuildPaths::objectiveCppExtensions() const noexcept
{
	return m_objectiveCppExts;
}
StringList BuildPaths::objectiveCxxExtensions() const noexcept
{
	return List::combine(m_objectiveCExts, m_objectiveCppExts);
}
const StringList& BuildPaths::resourceExtensions() const noexcept
{
	return m_resourceExts;
}

/*****************************************************************************/
void BuildPaths::setBuildDirectoriesBasedOnProjectKind(const SourceTarget& inProject)
{
	if (inProject.isSharedLibrary())
	{
		m_objDir = fmt::format("{}/obj.shared", m_buildOutputDir);
		// m_depDir = fmt::format("{}/dep.shared", m_buildOutputDir);
		m_asmDir = fmt::format("{}/asm.shared", m_buildOutputDir);
	}
	else
	{
		m_objDir = fmt::format("{}/obj", m_buildOutputDir);
		// m_depDir = fmt::format("{}/dep", m_buildOutputDir);
		m_asmDir = fmt::format("{}/asm", m_buildOutputDir);
	}

	m_depDir = m_objDir;
}

/*****************************************************************************/
SourceOutputs BuildPaths::getOutputs(const SourceTarget& inProject, const bool inDumpAssembly)
{
	SourceOutputs ret;

	chalet_assert(m_fileList.find(inProject.name()) != m_fileList.end(), "");

	SourceGroup files = std::move(*m_fileList.at(inProject.name()));
	SourceGroup directories = getDirectories(inProject);

	for (const auto& file : files.list)
	{
		auto ext = String::getPathSuffix(file);
		List::addIfDoesNotExist(ret.fileExtensions, std::move(ext));
	}

	const bool isNotMsvc = !m_state.compilers.isMsvc();
	ret.objectListLinker = getObjectFilesList(files.list, inProject);
	files.list = String::excludeIf(inProject.isSharedLibrary() ? m_fileListCacheShared : m_fileListCache, files.list);
	ret.groups = getSourceFileGroupList(std::move(files), inProject, inDumpAssembly);
	for (auto& group : ret.groups)
	{
		SourceType type = group->type;
		List::addIfDoesNotExist(ret.types, std::move(type));
	}

	StringList objSubDirs = getOutputDirectoryList(directories, objDir());
	// StringList depSubDirs = getOutputDirectoryList(directories, depDir());
	StringList depSubDirs;

	StringList asmSubDirs;
	if (inDumpAssembly)
	{
		asmSubDirs = getOutputDirectoryList(directories, asmDir());

		if (isNotMsvc)
			ret.directories.reserve(4 + objSubDirs.size() + depSubDirs.size() + asmSubDirs.size());
		else
			ret.directories.reserve(4 + objSubDirs.size() + asmSubDirs.size());
	}
	else
	{
		if (isNotMsvc)
			ret.directories.reserve(3 + objSubDirs.size() + depSubDirs.size());
		else
			ret.directories.reserve(3 + objSubDirs.size());
	}

	ret.directories.push_back(m_buildOutputDir);
	ret.directories.push_back(objDir());

#if defined(CHALET_WIN32) || defined(CHALET_LINUX)
	// m_intermediateDir is only used in windows so far
	if (!inProject.isStaticLibrary())
		ret.directories.push_back(m_intermediateDir);
#endif

	ret.directories.insert(ret.directories.end(), objSubDirs.begin(), objSubDirs.end());

	if (isNotMsvc)
	{
		ret.directories.push_back(depDir());
		ret.directories.insert(ret.directories.end(), depSubDirs.begin(), depSubDirs.end());
	}

	if (inDumpAssembly)
	{
		ret.directories.push_back(asmDir());
		ret.directories.insert(ret.directories.end(), asmSubDirs.begin(), asmSubDirs.end());
	}

	ret.target = getTargetFilename(inProject);

	return ret;
}

/*****************************************************************************/
void BuildPaths::setBuildEnvironment(const SourceOutputs& inOutput, const std::string& inHash) const
{
	auto objects = String::join(inOutput.objectListLinker);
	Environment::set(fmt::format("OBJS_{}", inHash).c_str(), objects);

	StringList depends;
	for (auto& group : inOutput.groups)
	{
		depends.push_back(group->dependencyFile);
	}

	auto depdendencies = String::join(std::move(depends));
	Environment::set(fmt::format("DEPS_{}", inHash).c_str(), depdendencies);
}

/*****************************************************************************/
void BuildPaths::replaceVariablesInPath(std::string& outPath, const std::string& inName) const
{
	const auto& buildDir = buildOutputDir();

	String::replaceAll(outPath, "${cwd}", m_inputs.workingDirectory());
	String::replaceAll(outPath, "${buildDir}", buildDir);

	const auto& externalDir = m_inputs.externalDirectory();
	if (!externalDir.empty())
	{
		String::replaceAll(outPath, "${externalDir}", externalDir);
		String::replaceAll(outPath, "${externalBuildDir}", fmt::format("{}/{}", buildDir, externalDir));
	}

	/*if (!inName.empty())
	{
		String::replaceAll(outPath, "${name}", inName);
	}*/
	UNUSED(inName);

	const auto& homeDirectory = m_inputs.homeDirectory();
	Environment::replaceCommonVariables(outPath, homeDirectory);
}

/*****************************************************************************/
std::string BuildPaths::getTargetFilename(const SourceTarget& inProject) const
{
	const auto& filename = inProject.outputFile();

	return fmt::format("{}/{}", buildOutputDir(), filename);
}

/*****************************************************************************/
std::string BuildPaths::getTargetBasename(const SourceTarget& inProject) const
{
	const auto& name = inProject.name();
	std::string base;

	if (!inProject.windowsPrefixOutputFilename())
	{
		base = name;
	}
	else
	{
		base = String::getPathFolderBaseName(inProject.outputFile());
	}

	return fmt::format("{}/{}", buildOutputDir(), base);
}

/*****************************************************************************/
std::string BuildPaths::getPrecompiledHeader(const SourceTarget& inProject) const
{
	std::string ret;
	if (inProject.usesPch())
	{
		ret = inProject.pch();
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getPrecompiledHeaderTarget(const SourceTarget& inProject) const
{
	std::string ret;
	if (inProject.usesPch())
	{
		std::string ext;
		if (m_state.compilers.isClangOrMsvc() || m_state.compilers.isIntelClassic())
			ext = "pch";
		// else if (m_state.compilers.isIntelClassic())
		// 	ext = "pchi";
		else
			ext = "gch";

		const std::string base = getPrecompiledHeaderInclude(inProject);
		ret = fmt::format("{}.{}", base, ext);
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getPrecompiledHeaderInclude(const SourceTarget& inProject) const
{
	std::string ret;
	if (inProject.usesPch())
	{
		const auto& pch = inProject.pch();
		ret = fmt::format("{}/{}", objDir(), pch);
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getWindowsManifestFilename(const SourceTarget& inProject) const
{
#if defined(CHALET_WIN32)
	if (!inProject.isStaticLibrary() && inProject.windowsApplicationManifestGenerationEnabled())
	{
		if (inProject.windowsApplicationManifest().empty())
		{
			auto outputFile = inProject.outputFileNoPrefix();

			// https://docs.microsoft.com/en-us/windows/win32/sbscs/application-manifests#file-name-syntax
			return fmt::format("{}/{}.manifest", intermediateDir(), outputFile);

			// return fmt::format("{}/default.manifest", intermediateDir());
		}
		else
		{
			return inProject.windowsApplicationManifest();
		}
	}
#else
	UNUSED(inProject);
#endif
	return std::string();
}

/*****************************************************************************/
std::string BuildPaths::getWindowsManifestResourceFilename(const SourceTarget& inProject) const
{
#if defined(CHALET_WIN32)
	if (!inProject.isStaticLibrary() && inProject.windowsApplicationManifestGenerationEnabled())
	{
		/*if (inProject.windowsApplicationManifest().empty())
		{
			return fmt::format("{}/default.manifest.rc", intermediateDir());
		}
		else*/
		{
			const auto& name = inProject.name();
			return fmt::format("{}/{}_manifest.rc", intermediateDir(), name);
		}
	}
#else
	UNUSED(inProject);
#endif

	return std::string();
}

/*****************************************************************************/
std::string BuildPaths::getWindowsIconResourceFilename(const SourceTarget& inProject) const
{
	std::string ret;

#if defined(CHALET_WIN32)
	if (inProject.isExecutable() && !inProject.windowsApplicationIcon().empty())
	{
		const auto& name = inProject.name();

		ret = fmt::format("{}/{}_win32_icon.rc", intermediateDir(), name);
	}
#else
	UNUSED(inProject);
#endif

	return ret;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
SourceFileGroupList BuildPaths::getSourceFileGroupList(SourceGroup&& inFiles, const SourceTarget& inProject, const bool inDumpAssembly)
{
	SourceFileGroupList ret;
	bool isMsvc = m_state.compilers.isMsvc();

	auto& fileListCache = inProject.isSharedLibrary() ? m_fileListCacheShared : m_fileListCache;

	for (auto& file : inFiles.list)
	{
		if (file.empty())
			continue;

		if (m_useCache)
			fileListCache.push_back(file);

		SourceType type = getSourceType(file);

		if (type == SourceType::Unknown)
			continue;

#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
		if (type == SourceType::WindowsResource)
			continue;
#endif
		auto group = std::make_unique<SourceFileGroup>();
		group->type = type;
		group->objectFile = getObjectFile(file, isMsvc);
		group->dependencyFile = getDependencyFile(file);
		group->sourceFile = std::move(file);

		ret.push_back(std::move(group));
	}

	// don't do this for the pch
	if (inDumpAssembly)
	{
		for (auto& group : ret)
		{
			group->assemblyFile = getAssemblyFile(group->sourceFile, isMsvc);
		}
	}

	// add the pch
	if (!inFiles.pch.empty())
	{
		auto group = std::make_unique<SourceFileGroup>();

		group->type = SourceType::CxxPrecompiledHeader;
		group->objectFile = getPrecompiledHeaderTarget(inProject);
		group->dependencyFile = getDependencyFile(inFiles.pch);
		group->sourceFile = std::move(inFiles.pch);

		ret.push_back(std::move(group));
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getObjectFile(const std::string& inSource, const bool inIsMsvc) const
{
	if (String::endsWith(m_resourceExts, inSource))
	{
#if defined(CHALET_WIN32)
		return fmt::format("{}/{}.res", objDir(), inSource);
#else
		return std::string();
#endif
	}
	else
	{
		return fmt::format("{}/{}.{}", objDir(), inSource, inIsMsvc ? "obj" : "o");
	}
}

/*****************************************************************************/
std::string BuildPaths::getAssemblyFile(const std::string& inSource, const bool inIsMsvc) const
{
	if (String::endsWith(m_resourceExts, inSource))
	{
		return std::string();
	}
	else
	{
		return fmt::format("{}/{}.{}.asm", asmDir(), inSource, inIsMsvc ? "obj" : "o");
	}
}

/*****************************************************************************/
std::string BuildPaths::getDependencyFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.d", depDir(), inSource);
}

/*****************************************************************************/
SourceType BuildPaths::getSourceType(const std::string& inSource) const
{
	const auto ext = String::getPathSuffix(inSource);
	if (!ext.empty())
	{
		if (String::equals(m_cppExts, ext))
		{
			return SourceType::CPlusPlus;
		}
		else if (String::equals(m_cExts, ext))
		{
			return SourceType::C;
		}
		else if (String::equals(m_resourceExts, ext))
		{
			return SourceType::WindowsResource;
		}
		else if (String::equals(m_objectiveCExts, ext))
		{
			return SourceType::ObjectiveC;
		}
		else if (String::equals(m_objectiveCppExts, ext))
		{
			return SourceType::ObjectiveCPlusPlus;
		}
	}

	return SourceType::Unknown;
}

/*****************************************************************************/
StringList BuildPaths::getObjectFilesList(const StringList& inFiles, const SourceTarget& inProject) const
{
	StringList ret;
	for (const auto& file : inFiles)
	{
		auto outFile = getObjectFile(file, m_state.compilers.isMsvc());
		if (!outFile.empty())
			ret.emplace_back(std::move(outFile));
	}

#if defined(CHALET_WIN32)
	if (m_state.compilers.isMsvc())
	{
		if (inProject.usesPch())
		{
			auto pchTarget = getPrecompiledHeaderTarget(inProject);
			String::replaceAll(pchTarget, ".pch", ".obj");

			ret.emplace_back(std::move(pchTarget));
		}
	}
#else
	UNUSED(inProject);
#endif

	return ret;
}

/*****************************************************************************/
StringList BuildPaths::getOutputDirectoryList(const SourceGroup& inDirectoryList, const std::string& inFolder) const
{
	StringList ret = inDirectoryList.list;
	std::for_each(ret.begin(), ret.end(), [inFolder](std::string& str) {
		str = fmt::format("{}/{}", inFolder, str);
	});

	return ret;
}

/*****************************************************************************/
StringList BuildPaths::getFileList(const SourceTarget& inProject) const
{
	auto manifestResource = getWindowsManifestResourceFilename(inProject);
	auto iconResource = getWindowsIconResourceFilename(inProject);

	StringList extensions = List::combine(m_cExts, m_cppExts, m_resourceExts, m_objectiveCExts, m_objectiveCppExts);

	const auto& files = inProject.files();
	if (files.size() > 0)
	{
		auto& pch = inProject.pch();
		bool usesPch = inProject.usesPch();
		StringList fileList;

		for (auto& file : files)
		{
			if (usesPch && String::equals(pch, file))
			{
				Diagnostic::warn("Precompiled header explicitly included in 'files': {} (ignored)", file);
				continue;
			}

			if (!String::endsWith(extensions, file))
			{
				Diagnostic::warn("File type in 'files' is not required or supported: {} (ignored)", file);
				continue;
			}

			if (!Commands::pathExists(file))
			{
				Diagnostic::warn("File not found: {}", file);
				continue;
			}

			List::addIfDoesNotExist(fileList, file);
		}

		if (!manifestResource.empty())
		{
			List::addIfDoesNotExist(fileList, std::move(manifestResource));
		}

		if (!iconResource.empty())
		{
			List::addIfDoesNotExist(fileList, std::move(iconResource));
		}

		return fileList;
	}

	const auto& locations = inProject.locations();

	const auto searchString = "." + String::join(extensions, " .");

	const auto& excludesList = inProject.locationExcludes();
	std::string excludes = String::join(excludesList);
	Path::sanitize(excludes);

	StringList ret;

	CHALET_TRY
	{
		for (auto& locRaw : locations)
		{
			std::string loc = locRaw;
			Path::sanitize(loc);

			if (String::equals(m_intermediateDir, locRaw))
				continue;

			if (!Commands::pathExists(loc))
			{
				Diagnostic::warn("Path not found: {}", loc);
				continue;
			}

			int j = 0;
			for (auto& item : fs::recursive_directory_iterator(loc))
			{
				if (item.is_directory())
					continue;

				const auto& path = item.path();

				if (!path.has_extension())
					continue;

				const std::string ext = path.extension().string();
				if (!String::contains(ext, searchString))
					continue;

				std::string source = path.string();
				Path::sanitize(source, true);

				if (String::contains(source, excludes))
					continue;

				bool excluded = false;
				for (auto& exclude : excludesList)
				{
					if (String::contains(exclude, source))
					{
						excluded = true;
						break;
					}
				}
				if (excluded)
					continue;

				List::addIfDoesNotExist(ret, std::move(source));
				j++;
			}
		}
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR(err.what());
		ret.clear();
		return ret;
	}

	if (!manifestResource.empty())
	{
		List::addIfDoesNotExist(ret, std::move(manifestResource));
	}
	if (!iconResource.empty())
	{
		List::addIfDoesNotExist(ret, std::move(iconResource));
	}

	return ret;
}

/*****************************************************************************/
StringList BuildPaths::getDirectoryList(const SourceTarget& inProject) const
{
	StringList ret;

	CHALET_TRY
	{
		if (inProject.usesPch())
		{
			if (Commands::pathExists(inProject.pch()))
			{
				std::string outPath = String::getPathFolder(inProject.pch());
				Path::sanitize(outPath, true);

#if defined(CHALET_MACOS)
				if (!m_inputs.universalArches().empty())
				{
					for (auto& arch : m_inputs.universalArches())
					{
						auto path = fmt::format("{}_{}", outPath, arch);
						ret.emplace_back(std::move(path));
					}
				}
#endif
				ret.emplace_back(std::move(outPath));
			}
		}

		const auto& files = inProject.files();
		if (files.size() > 0)
		{
			for (auto& file : files)
			{
				if (!Commands::pathExists(file))
					continue;

				std::string outPath = String::getPathFolder(file);
				Path::sanitize(outPath, true);

				ret.emplace_back(std::move(outPath));
			}

			return ret;
		}

		const auto& locations = inProject.locations();
		const auto& locationExcludes = inProject.locationExcludes();

		std::string excludes = String::join(locationExcludes);
		Path::sanitize(excludes);

		for (auto& locRaw : locations)
		{
			std::string loc = locRaw;
			Path::sanitize(loc);

			// if (m_useCache && List::contains(m_directoryCache, loc))
			// 	continue;

			ret.push_back(loc);

			if (String::equals(m_intermediateDir, locRaw))
				continue;

			if (!Commands::pathExists(loc))
			{
				Diagnostic::warn("Path not found: {}", loc);
				continue;
			}

			for (auto& item : fs::recursive_directory_iterator(loc))
			{
				if (!item.is_directory())
					continue;

				std::string path = item.path().string();
				Path::sanitize(path, true);

				if (String::contains(path, excludes))
					continue;

				bool excluded = false;
				for (auto& exclude : locationExcludes)
				{
					if (String::contains(exclude, path))
					{
						excluded = true;
						break;
					}
				}
				if (excluded)
					continue;

				ret.push_back(path);
			}

			// if (m_useCache && !List::contains(m_directoryCache, loc))
			// 	m_directoryCache.push_back(loc);
		}
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR(err.what());
		ret.clear();
		return ret;
	}

	return ret;
}

/*****************************************************************************/
BuildPaths::SourceGroup BuildPaths::getFiles(const SourceTarget& inProject) const
{
	SourceGroup ret;
	ret.list = getFileList(inProject);
	ret.pch = getPrecompiledHeader(inProject);

	return ret;
}

/*****************************************************************************/
BuildPaths::SourceGroup BuildPaths::getDirectories(const SourceTarget& inProject) const
{
	SourceGroup ret;
	ret.list = getDirectoryList(inProject);

	return ret;
}
}
