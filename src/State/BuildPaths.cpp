/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildPaths.hpp"

#include "Libraries/Format.hpp"
#include "State/WorkspaceInfo.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
BuildPaths::BuildPaths(const CommandLineInputs& inInputs, const WorkspaceInfo& inInfo) :
	m_inputs(inInputs),
	m_info(inInfo)
{
}

/*****************************************************************************/
void BuildPaths::initialize()
{
	chalet_assert(!m_initialized, "BuildPaths::initialize called twice.");

	// m_configuration = fmt::format("{}_{}_{}", m_info.hostArchitectureString(), m_info.targetArchitectureString(), inBuildConfiguration);
	m_configuration = fmt::format("{}_{}", m_info.targetArchitectureString(), m_info.buildConfiguration());
	m_buildOutputDir = fmt::format("{}/{}", m_inputs.buildPath(), m_configuration);
	m_objDir = fmt::format("{}/obj", m_buildOutputDir);
	m_depDir = fmt::format("{}/dep", m_buildOutputDir);
	m_asmDir = fmt::format("{}/asm", m_buildOutputDir);

	m_initialized = true;
}

/*****************************************************************************/
const std::string& BuildPaths::workingDirectory() const noexcept
{
	return m_workingDirectory;
}

void BuildPaths::setWorkingDirectory(const std::string& inValue)
{
	chalet_assert(m_workingDirectory.empty(), "");

	if (Commands::pathExists(inValue))
	{
		m_workingDirectory = inValue;
	}
	else
	{
		auto workingDirectory = Commands::getWorkingDirectory();
		Path::sanitize(workingDirectory, true);

		m_workingDirectory = workingDirectory;
	}
}

/*****************************************************************************/
const std::string& BuildPaths::externalDepDir() const noexcept
{
	return m_externalDepDir;
}

void BuildPaths::setExternalDepDir(const std::string& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_externalDepDir = inValue;

	if (m_externalDepDir.back() == '/')
		m_externalDepDir.pop_back();
}

/*****************************************************************************/
const std::string& BuildPaths::buildPath() const
{
	const auto& buildPath = m_inputs.buildPath();
	chalet_assert(!buildPath.empty(), "buildPath was not defined");
	if (!m_binDirMade)
	{
		if (!Commands::pathExists(buildPath))
		{
			m_binDirMade = Commands::makeDirectory(buildPath);
		}
		else
		{
			m_binDirMade = true;
		}
	}

	return buildPath;
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

/*****************************************************************************/
SourceOutputs BuildPaths::getOutputs(const ProjectTarget& inProject, const bool inIsMsvc, const bool inDumpAssembly, const bool inObjExtension) const
{
	SourceOutputs ret;

	SourceGroup files = getFiles(inProject);
	SourceGroup directories = getDirectories(inProject);

	ret.objectListLinker = getObjectFilesList(files.list, inObjExtension);

#if defined(CHALET_WIN32)
	if (inIsMsvc)
	{
		if (inProject.usesPch())
		{
			auto pchTarget = getPrecompiledHeaderTarget(inProject, true);
			String::replaceAll(pchTarget, ".pch", ".obj");

			ret.objectListLinker.push_back(std::move(pchTarget));
		}
	}
#else
	UNUSED(inIsMsvc);
#endif

	ret.objectList = getObjectFilesList(String::excludeIf(m_fileListCache, files.list), inObjExtension);

	for (const auto& file : files.list)
	{
		auto ext = String::getPathSuffix(file);
		List::addIfDoesNotExist(ret.fileExtensions, std::move(ext));

		if (m_useCache && !List::contains(m_fileListCache, file))
			m_fileListCache.push_back(file);
	}

	StringList objSubDirs = getOutputDirectoryList(directories, m_objDir);

	ret.dependencyList = getDependencyFilesList(files);
	StringList depSubDirs = getOutputDirectoryList(directories, m_depDir);

	StringList asmSubDirs;
	if (inDumpAssembly)
	{
		ret.assemblyList = getAssemblyFilesList(files, inObjExtension);

		asmSubDirs = getOutputDirectoryList(directories, m_asmDir);

		if (!inIsMsvc)
			ret.directories.reserve(4 + objSubDirs.size() + depSubDirs.size() + asmSubDirs.size());
		else
			ret.directories.reserve(4 + objSubDirs.size() + asmSubDirs.size());
	}
	else
	{
		if (!inIsMsvc)
			ret.directories.reserve(3 + objSubDirs.size() + depSubDirs.size());
		else
			ret.directories.reserve(3 + objSubDirs.size());
	}

	ret.directories.push_back(m_buildOutputDir);
	ret.directories.push_back(m_objDir);
	ret.directories.insert(ret.directories.end(), objSubDirs.begin(), objSubDirs.end());

	if (!inIsMsvc)
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
void BuildPaths::setBuildEnvironment(const SourceOutputs& inOutput, const std::string& inHash, const bool inDumpAssembly) const
{
	auto objects = String::join(inOutput.objectListLinker);
	Environment::set(fmt::format("SOURCE_OBJS_{}", inHash).c_str(), objects);

	auto depdendencies = String::join(inOutput.dependencyList);
	Environment::set(fmt::format("SOURCE_DEPS", inHash).c_str(), depdendencies);

	if (inDumpAssembly)
	{
		auto assemblies = String::join(inOutput.assemblyList);
		Environment::set(fmt::format("SOURCE_ASMS", inHash).c_str(), assemblies);
	}

	// UNUSED(inOutput, inDumpAssembly);
}

/*****************************************************************************/
void BuildPaths::parsePathWithVariables(std::string& outPath, const std::string& inName) const
{
	const auto& buildDir = buildOutputDir();

	String::replaceAll(outPath, "${buildDir}", buildDir);

	const auto& external = externalDepDir();
	if (!external.empty())
	{
		String::replaceAll(outPath, "${externalDepDir}", external);
	}

	if (!inName.empty())
	{
		String::replaceAll(outPath, "${name}", inName);
	}

	Path::sanitize(outPath);
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
/*****************************************************************************/
/*****************************************************************************/
StringList BuildPaths::getObjectFilesList(const StringList& inFiles, const bool inObjExtension) const
{
	StringList ret;
	auto ext = inObjExtension ? "obj" : "o";
	for (const auto& file : inFiles)
	{
		if (!String::endsWith(".rc", file))
		{
			ret.push_back(fmt::format("{}/{}.{}", m_objDir, file, ext));
		}
		else
		{
#if defined(CHALET_WIN32)
			ret.push_back(fmt::format("{}/{}.res", m_objDir, file));
#else
			continue;
#endif
		}
	}

	return ret;
}

/*****************************************************************************/
StringList BuildPaths::getDependencyFilesList(const SourceGroup& inFiles) const
{
	StringList ret;
	for (const auto& file : inFiles.list)
	{
		if (file.empty())
			continue;

		ret.push_back(fmt::format("{}/{}.d", m_depDir, file));
	}

	if (!inFiles.pch.empty())
		ret.push_back(fmt::format("{}/{}.d", m_depDir, inFiles.pch));

	return ret;
}

/*****************************************************************************/
StringList BuildPaths::getAssemblyFilesList(const SourceGroup& inFiles, const bool inObjExtension) const
{
	StringList ret;
	auto ext = inObjExtension ? "obj" : "o";
	for (auto& file : inFiles.list)
	{
		if (!String::endsWith(".rc", file))
		{
			ret.push_back(fmt::format("{}/{}.{}.asm", m_asmDir, file, ext));
		}
	}

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
				Diagnostic::warn(fmt::format("Precompiled header explicitly included as file: {} (ignored)", file));
				continue;
			}

			if (!Commands::pathExists(file))
			{
				Diagnostic::warn(fmt::format("File not found: {}", file));
				continue;
			}

			fileList.push_back(file);
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
	for (auto& locRaw : locations)
	{
		std::string loc = locRaw;
		Path::sanitize(loc);

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

	return ret;
}

/*****************************************************************************/
StringList BuildPaths::getDirectoryList(const ProjectTarget& inProject) const
{
	StringList ret;

	if (inProject.usesPch())
	{
		fs::path pch{ inProject.pch() };
		if (Commands::pathExists(pch))
		{
			std::string outPath = pch.parent_path().string();
			Path::sanitize(outPath, true);

			ret.push_back(std::move(outPath));
		}
	}

	const auto& files = inProject.files();
	if (files.size() > 0)
	{
		for (auto& file : files)
		{
			fs::path path{ file };
			if (!Commands::pathExists(path))
				continue;

			std::string outPath = path.parent_path().string();
			Path::sanitize(outPath, true);

			ret.push_back(std::move(outPath));
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
