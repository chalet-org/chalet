/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildPaths.hpp"

#include "Compile/CompilerConfig.hpp"
#include "State/BuildInfo.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
BuildPaths::BuildPaths(const CommandLineInputs& inInputs, const WorkspaceEnvironment& inEnvironment) :
	m_inputs(inInputs),
	m_environment(inEnvironment),
	m_cExts({ ".c", ".C" }),
	m_cppExts({ ".cpp", ".cc", ".cxx", ".c++", ".CPP", ".CC", ".CXX", ".C++" }),
	m_resourceExts({ ".rc", ".RC" }),
	m_objectiveCExts({ ".m", ".M" }),
	m_objectiveCppExts({ ".mm" })
{
}

/*****************************************************************************/
void BuildPaths::initialize(const BuildInfo& inInfo)
{
	chalet_assert(!m_initialized, "BuildPaths::initialize called twice.");

	const auto& outputDirectory = m_inputs.outputDirectory();
	if (!Commands::pathExists(outputDirectory))
	{
		Commands::makeDirectory(outputDirectory);
	}

	// m_configuration = fmt::format("{}_{}_{}", inInfo.hostArchitectureString(), inInfo.targetArchitectureString(), inBuildConfiguration);
	auto arch = m_inputs.getArchWithOptionsAsString(inInfo.targetArchitectureString());

	m_configuration = fmt::format("{}_{}", arch, inInfo.buildConfiguration());
	m_buildOutputDir = fmt::format("{}/{}", outputDirectory, m_configuration);
	m_objDir = fmt::format("{}/obj", m_buildOutputDir);
	m_depDir = fmt::format("{}/dep", m_buildOutputDir);
	m_asmDir = fmt::format("{}/asm", m_buildOutputDir);
	m_intermediateDir = fmt::format("{}/intermediate", outputDirectory);

	{
		auto search = m_intermediateDir.find_first_not_of("./"); // if it's relative
		m_intermediateDir = m_intermediateDir.substr(search);
	}

	m_initialized = true;
}

/*****************************************************************************/
void BuildPaths::populateFileList(const ProjectTarget& inProject)
{
	if (m_fileList.find(inProject.name()) != m_fileList.end())
		return;

	SourceGroup files = getFiles(inProject);

	for (auto& file : files.list)
	{
		auto ext = String::getPathSuffix(file);
		List::addIfDoesNotExist(m_allFileExtensions, std::move(ext));
	}

	std::sort(m_allFileExtensions.begin(), m_allFileExtensions.end());

	m_fileList.emplace(inProject.name(), std::make_unique<SourceGroup>(std::move(files)));
}

const std::string& BuildPaths::homeDirectory() const noexcept
{
	return m_inputs.homeDirectory();
}

/*****************************************************************************/
const std::string& BuildPaths::outputDirectory() const
{
	return m_inputs.outputDirectory();
}

const std::string& BuildPaths::buildOutputDir() const noexcept
{
	chalet_assert(m_initialized, "BuildPaths::buildOutputDir() called before BuildPaths::initialize().");
	return m_buildOutputDir;
}

const std::string& BuildPaths::configuration() const noexcept
{
	chalet_assert(m_initialized, "BuildPaths::configuration() called before BuildPaths::initialize().");
	return m_configuration;
}

const std::string& BuildPaths::objDir() const noexcept
{
	chalet_assert(m_initialized, "BuildPaths::objDir() called before BuildPaths::initialize().");
	return m_objDir;
}

const std::string& BuildPaths::depDir() const noexcept
{
	chalet_assert(m_initialized, "BuildPaths::depDir() called before BuildPaths::initialize().");
	return m_depDir;
}

const std::string& BuildPaths::asmDir() const noexcept
{
	chalet_assert(m_initialized, "BuildPaths::asmDir() called before BuildPaths::initialize().");
	return m_asmDir;
}

const std::string& BuildPaths::intermediateDir() const noexcept
{
	chalet_assert(m_initialized, "BuildPaths::intermediateDir() called before BuildPaths::initialize().");
	return m_intermediateDir;
}

/*****************************************************************************/
const StringList& BuildPaths::allFileExtensions() const noexcept
{
	return m_allFileExtensions;
}

/*****************************************************************************/
SourceOutputs BuildPaths::getOutputs(const ProjectTarget& inProject, const CompilerConfig& inConfig, const bool inDumpAssembly) const
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

	const bool isMsvc = inConfig.isMsvc();
	const bool isNotMsvc = !isMsvc;
	ret.objectListLinker = getObjectFilesList(files.list, inProject, isMsvc);
	files.list = String::excludeIf(m_fileListCache, files.list);
	ret.groups = getSourceFileGroupList(std::move(files), inProject, inConfig, inDumpAssembly);
	for (auto& group : ret.groups)
	{
		SourceType type = group->type;
		List::addIfDoesNotExist(ret.types, std::move(type));
	}

	StringList objSubDirs = getOutputDirectoryList(directories, m_objDir);
	StringList depSubDirs = getOutputDirectoryList(directories, m_depDir);

	StringList asmSubDirs;
	if (inDumpAssembly)
	{
		asmSubDirs = getOutputDirectoryList(directories, m_asmDir);

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
	ret.directories.push_back(m_objDir);

#if !defined(CHALET_MACOS)
	// m_intermediateDir is only used in windows so far
	if (!inProject.isStaticLibrary())
		ret.directories.push_back(m_intermediateDir);
#endif

	ret.directories.insert(ret.directories.end(), objSubDirs.begin(), objSubDirs.end());

	if (isNotMsvc)
	{
		ret.directories.push_back(m_depDir);
		ret.directories.insert(ret.directories.end(), depSubDirs.begin(), depSubDirs.end());
	}

	if (inDumpAssembly)
	{
		ret.directories.push_back(m_asmDir);
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

	auto depdendencies = String::join(depends);
	Environment::set(fmt::format("DEPS_{}", inHash).c_str(), depdendencies);
}

/*****************************************************************************/
void BuildPaths::replaceVariablesInPath(std::string& outPath, const std::string& inName) const
{
	const auto& buildDir = buildOutputDir();

	String::replaceAll(outPath, "${buildDir}", buildDir);

	const auto& external = m_environment.externalDepDir();
	if (!external.empty())
	{
		String::replaceAll(outPath, "${externalDepDir}", external);
		String::replaceAll(outPath, "${external}", external);
		String::replaceAll(outPath, "${vendor}", external);
	}

	if (!inName.empty())
	{
		String::replaceAll(outPath, "${name}", inName);
	}

	const auto& homeDirectory = m_inputs.homeDirectory();
	Environment::replaceCommonVariables(outPath, homeDirectory);
}

/*****************************************************************************/
std::string BuildPaths::getTargetFilename(const ProjectTarget& inProject) const
{
	const auto& filename = inProject.outputFile();

	return fmt::format("{}/{}", buildOutputDir(), filename);
}

/*****************************************************************************/
std::string BuildPaths::getTargetBasename(const ProjectTarget& inProject) const
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
std::string BuildPaths::getPrecompiledHeader(const ProjectTarget& inProject) const
{
	std::string ret;
	if (inProject.usesPch())
	{
		ret = inProject.pch();
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getPrecompiledHeaderTarget(const ProjectTarget& inProject, const bool inPchExtension) const
{
	std::string ret;
	if (inProject.usesPch())
	{
		auto ext = inPchExtension ? "pch" : "gch";

		const std::string base = getPrecompiledHeaderInclude(inProject);
		ret = fmt::format("{base}.{ext}",
			FMT_ARG(base),
			FMT_ARG(ext));
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getPrecompiledHeaderInclude(const ProjectTarget& inProject) const
{
	std::string ret;
	if (inProject.usesPch())
	{
		const auto& pch = inProject.pch();
		ret = fmt::format("{objDir}/{pch}",
			fmt::arg("objDir", objDir()),
			FMT_ARG(pch));
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getWindowsManifestFilename(const ProjectTarget& inProject) const
{
#if defined(CHALET_WIN32)
	if (inProject.windowsApplicationManifest().empty())
	{
		std::string ret;

		if (!inProject.isStaticLibrary())
		{
			auto outputFile = inProject.outputFileNoPrefix();

			// https://docs.microsoft.com/en-us/windows/win32/sbscs/application-manifests#file-name-syntax
			ret = fmt::format("{}/{}.manifest", m_intermediateDir, outputFile);
		}
		return ret;
	}

	return inProject.windowsApplicationManifest();
#else
	UNUSED(inProject);
	return std::string();
#endif
}

/*****************************************************************************/
std::string BuildPaths::getWindowsManifestResourceFilename(const ProjectTarget& inProject) const
{
	std::string ret;

#if defined(CHALET_WIN32)
	if (!inProject.isStaticLibrary())
	{
		const auto& name = inProject.name();

		ret = fmt::format("{}/{}_win32_manifest.rc", m_intermediateDir, name);
	}
#else
	UNUSED(inProject);
#endif

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getWindowsIconResourceFilename(const ProjectTarget& inProject) const
{
	std::string ret;

#if defined(CHALET_WIN32)
	if (inProject.isExecutable() && !inProject.windowsApplicationIcon().empty())
	{
		const auto& name = inProject.name();

		ret = fmt::format("{}/{}_win32_icon.rc", m_intermediateDir, name);
	}
#else
	UNUSED(inProject);
#endif

	return ret;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
SourceFileGroupList BuildPaths::getSourceFileGroupList(SourceGroup&& inFiles, const ProjectTarget& inProject, const CompilerConfig& inConfig, const bool inDumpAssembly) const
{
	SourceFileGroupList ret;
	bool isMsvc = inConfig.isMsvc();

	for (auto& file : inFiles.list)
	{
		if (file.empty())
			continue;

		if (m_useCache)
			m_fileListCache.push_back(file);

		SourceType type = getSourceType(file);

		if (type == SourceType::Unknown)
			continue;

#if !defined(CHALET_WIN32)
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
		group->objectFile = getPrecompiledHeaderTarget(inProject, inConfig.isClangOrMsvc());
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
		return fmt::format("{}/{}.res", m_objDir, inSource);
	}
	else
	{
		return fmt::format("{}/{}.{}", m_objDir, inSource, inIsMsvc ? "obj" : "o");
	}

	return std::string();
}

/*****************************************************************************/
std::string BuildPaths::getAssemblyFile(const std::string& inSource, const bool inIsMsvc) const
{
	if (String::endsWith(m_resourceExts, inSource))
	{
	}
	else
	{
		return fmt::format("{}/{}.{}.asm", m_asmDir, inSource, inIsMsvc ? "obj" : "o");
	}

	return std::string();
}

/*****************************************************************************/
std::string BuildPaths::getDependencyFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.d", m_depDir, inSource);
}

/*****************************************************************************/
SourceType BuildPaths::getSourceType(const std::string& inSource) const
{
	if (String::endsWith(m_cppExts, inSource))
	{
		return SourceType::CPlusPlus;
	}
	else if (String::endsWith(m_cExts, inSource))
	{
		return SourceType::C;
	}
	else if (String::endsWith(m_resourceExts, inSource))
	{
		return SourceType::WindowsResource;
	}
	else if (String::endsWith(m_objectiveCExts, inSource))
	{
		return SourceType::ObjectiveC;
	}
	else if (String::endsWith(m_objectiveCppExts, inSource))
	{
		return SourceType::ObjectiveCPlusPlus;
	}

	return SourceType::Unknown;
}

/*****************************************************************************/
StringList BuildPaths::getObjectFilesList(const StringList& inFiles, const ProjectTarget& inProject, const bool inIsMsvc) const
{
	StringList ret;
	auto ext = inIsMsvc ? "obj" : "o";
	for (const auto& file : inFiles)
	{
		if (!String::endsWith(m_resourceExts, file))
		{
			ret.emplace_back(fmt::format("{}/{}.{}", m_objDir, file, ext));
		}
		else
		{
#if defined(CHALET_WIN32)
			ret.emplace_back(fmt::format("{}/{}.res", m_objDir, file));
#endif
		}
	}

#if defined(CHALET_WIN32)
	if (inIsMsvc)
	{
		if (inProject.usesPch())
		{
			auto pchTarget = getPrecompiledHeaderTarget(inProject, true);
			String::replaceAll(pchTarget, ".pch", ".obj");

			ret.emplace_back(std::move(pchTarget));
		}
	}
#else
	UNUSED(inProject, inIsMsvc);
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
StringList BuildPaths::getFileList(const ProjectTarget& inProject) const
{
	auto manifestResource = getWindowsManifestResourceFilename(inProject);
	auto iconResource = getWindowsIconResourceFilename(inProject);

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
				Diagnostic::warn("Precompiled header explicitly included as file: {} (ignored)", file);
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

	auto searchString = String::join(inProject.fileExtensions());
	searchString += " " + String::toUpperCase(searchString);

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

			// if (m_useCache && List::contains(m_fileListCache, loc))
			// 	continue;

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

			// if (m_useCache && !List::contains(m_fileListCache, source))
			// 	m_fileListCache.push_back(source);
		}
	}
	CHALET_CATCH(const fs::filesystem_error& err)
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
StringList BuildPaths::getDirectoryList(const ProjectTarget& inProject) const
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
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what());
		ret.clear();
		return ret;
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getPrecompiledHeaderDirectory(const ProjectTarget& inProject) const
{
	std::string ret;
	if (inProject.usesPch())
	{
		ret = String::getPathFolder(inProject.pch());
	}

	return ret;
}

/*****************************************************************************/
BuildPaths::SourceGroup BuildPaths::getFiles(const ProjectTarget& inProject) const
{
	SourceGroup ret;
	ret.list = getFileList(inProject);
	ret.pch = getPrecompiledHeader(inProject);

	return ret;
}

/*****************************************************************************/
BuildPaths::SourceGroup BuildPaths::getDirectories(const ProjectTarget& inProject) const
{
	SourceGroup ret;
	ret.list = getDirectoryList(inProject);
	ret.pch = getPrecompiledHeaderDirectory(inProject);

	return ret;
}
}
