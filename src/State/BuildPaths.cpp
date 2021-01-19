/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildPaths.hpp"

#include <unistd.h>

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
void BuildPaths::initialize(const std::string& inBuildConfiguration)
{
	chalet_assert(!m_initialized, "BuildPaths::initialize called twice.");

	m_buildOutputDir = fmt::format("{}/{}", m_buildDir, inBuildConfiguration);
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

	auto workingDirectory = Commands::getWorkingDirectory();
	Path::sanitize(workingDirectory, true);

	if (Commands::pathExists(inValue))
	{
		m_workingDirectory = inValue;

		if (m_workingDirectory != workingDirectory)
		{
			if (chdir(m_workingDirectory.c_str()) != 0)
			{
				Diagnostic::error(fmt::format("Error changing directory to '{}'", m_workingDirectory));
			}
		}
	}
	else
	{
		m_workingDirectory = workingDirectory;
	}
}

/*****************************************************************************/
const std::string& BuildPaths::buildDir() const
{
	chalet_assert(!m_buildDir.empty(), "buildDir was not defined");
	if (!m_binDirMade)
	{
		if (!Commands::pathExists(m_buildDir))
		{
			m_binDirMade = Commands::makeDirectory(m_buildDir);
		}
		else
		{
			m_binDirMade = true;
		}
	}

	return m_buildDir;
}

const std::string& BuildPaths::buildOutputDir() const noexcept
{
	chalet_assert(m_initialized, "BuildPaths::buildOutputDir() called before BuildPaths::initialize().");
	return m_buildOutputDir;
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
SourceOutputs BuildPaths::getOutputs(const ProjectConfiguration& inProject) const
{
	SourceOutputs ret;

	const bool dumpAssembly = inProject.dumpAssembly();

	SourceGroup files = getFiles(inProject);
	SourceGroup directories = getDirectories(inProject);

	for (auto& file : files.list)
	{
		std::string ext = String::getPathSuffix(file);
		List::addIfDoesNotExist(ret.fileExtensions, std::move(ext));
	}

	ret.objectList = getObjectFilesList(files);
	StringList objSubDirs = getOutputDirectoryList(directories, m_objDir);

	ret.dependencyList = getDependencyFilesList(files);
	StringList depSubDirs = getOutputDirectoryList(directories, m_depDir);

	StringList asmSubDirs;
	if (dumpAssembly)
	{
		ret.assemblyList = getAssemblyFilesList(files);
		asmSubDirs = getOutputDirectoryList(directories, m_asmDir);

		ret.directories.reserve(4 + objSubDirs.size() + depSubDirs.size() + asmSubDirs.size());
	}
	else
	{
		ret.directories.reserve(3 + objSubDirs.size() + depSubDirs.size());
	}

	ret.directories.push_back(m_buildOutputDir);
	ret.directories.push_back(m_objDir);
	ret.directories.push_back(m_depDir);

	ret.directories.insert(ret.directories.end(), objSubDirs.begin(), objSubDirs.end());
	ret.directories.insert(ret.directories.end(), depSubDirs.begin(), depSubDirs.end());

	if (dumpAssembly)
	{
		ret.directories.push_back(m_asmDir);
		ret.directories.insert(ret.directories.end(), asmSubDirs.begin(), asmSubDirs.end());
	}

	return ret;
}

/*****************************************************************************/
void BuildPaths::setBuildEnvironment(const SourceOutputs& inOutput, const bool inDumpAssembly) const
{
	auto objects = String::join(inOutput.objectList, " ");
	Environment::set("SOURCE_OBJS", objects);

	auto depdendencies = String::join(inOutput.dependencyList, " ");
	Environment::set("SOURCE_DEPS", depdendencies);

	if (inDumpAssembly)
	{
		auto assemblies = String::join(inOutput.assemblyList, " ");
		Environment::set("SOURCE_ASMS", assemblies);
	}
}

/*****************************************************************************/
std::string BuildPaths::getTargetFilename(const ProjectConfiguration& inProject) const
{
	const auto& filename = inProject.outputFile();

	return fmt::format("{}/{}", buildOutputDir(), filename);
}

/*****************************************************************************/
std::string BuildPaths::getTargetBasename(const ProjectConfiguration& inProject) const
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
std::string BuildPaths::getPrecompiledHeader(const ProjectConfiguration& inProject) const
{
	std::string ret;
	if (inProject.usesPch())
	{
		ret = inProject.pch();
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getPrecompiledHeaderTarget(const ProjectConfiguration& inProject, const bool inIsClang) const
{
	std::string ret;
	if (inProject.usesPch())
	{
		auto ext = inIsClang ? "pch" : "gch";

		const std::string base = getPrecompiledHeaderInclude(inProject);
		ret = fmt::format("{base}.{ext}",
			FMT_ARG(base),
			FMT_ARG(ext));
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getPrecompiledHeaderInclude(const ProjectConfiguration& inProject) const
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
StringList BuildPaths::getObjectFilesList(const SourceGroup& inFiles) const
{
	StringList ret = inFiles.list;
	std::for_each(ret.begin(), ret.end(), [this](std::string& str) {
		if (!String::endsWith(".rc", str))
		{
			str = fmt::format("{}/{}.o", m_objDir, str);
		}
		else
		{
#if defined(CHALET_WIN32)
			str = fmt::format("{}/{}.res", m_objDir, str);
#else
			str = "";
#endif
		}
	});

	return ret;
}

/*****************************************************************************/
StringList BuildPaths::getDependencyFilesList(const SourceGroup& inFiles) const
{
	StringList ret = inFiles.list;
	std::for_each(ret.begin(), ret.end(), [this](std::string& str) {
		str = fmt::format("{}/{}.d", m_depDir, str);
	});

	ret.push_back(fmt::format("{}/{}.d", m_depDir, inFiles.pch));

	return ret;
}

/*****************************************************************************/
StringList BuildPaths::getAssemblyFilesList(const SourceGroup& inFiles) const
{
	StringList ret = inFiles.list;
	std::for_each(ret.begin(), ret.end(), [this](std::string& str) {
		if (!String::endsWith(".rc", str))
		{
			str = fmt::format("{}/{}.o.asm", m_asmDir, str);
		}
		else
		{
			str = "";
		}
	});

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
StringList BuildPaths::getFileList(const ProjectConfiguration& inProject) const
{
	const auto& files = inProject.files();
	if (files.size() > 0)
	{
		StringList fileList;

		for (auto& file : files)
		{
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

	const std::string searchString = String::join(inProject.fileExtensions(), " ");

	const auto& excludesList = inProject.locationExcludes();
	std::string excludes = String::join(excludesList, " ");
	Path::sanitize(excludes);

	StringList ret;
	for (auto& locRaw : locations)
	{
		std::string loc = locRaw;
		Path::sanitize(loc);

		// if (inCheckExisting && List::contains(outCache, loc))
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

			ret.push_back(source);
			j++;
		}

		// if (inCheckExisting && !List::contains(outCache, loc))
		// 	outCache.push_back(loc);
	}

	return ret;
}

/*****************************************************************************/
StringList BuildPaths::getDirectoryList(const ProjectConfiguration& inProject) const
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

	std::string excludes = String::join(locationExcludes, " ");
	Path::sanitize(excludes);

	for (auto& locRaw : locations)
	{
		std::string loc = locRaw;
		Path::sanitize(loc);

		// if (inCheckExisting && List::contains(outCache, loc))
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

		// if (inCheckExisting && !List::contains(outCache, loc))
		// 	outCache.push_back(loc);
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getPrecompiledHeaderDirectory(const ProjectConfiguration& inProject) const
{
	std::string ret;
	if (inProject.usesPch())
	{
		ret = String::getPathFolder(inProject.pch());
	}

	return ret;
}

/*****************************************************************************/
BuildPaths::SourceGroup BuildPaths::getFiles(const ProjectConfiguration& inProject) const
{
	SourceGroup ret;
	ret.list = getFileList(inProject);
	ret.pch = getPrecompiledHeader(inProject);

	return ret;
}

/*****************************************************************************/
BuildPaths::SourceGroup BuildPaths::getDirectories(const ProjectConfiguration& inProject) const
{
	SourceGroup ret;
	ret.list = getDirectoryList(inProject);
	ret.pch = getPrecompiledHeaderDirectory(inProject);

	return ret;
}
}
