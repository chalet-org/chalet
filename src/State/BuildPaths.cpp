/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildPaths.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildConfiguration.hpp"
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
BuildPaths::BuildPaths(const BuildState& inState) :
	m_state(inState),
	m_cExts({
		"c",
	}),
	m_resourceExts({
		"rc",
		"RC",
	}),
	m_objectiveCExts({
		"m",
		"M",
	}),
	m_objectiveCppExts({
		"mm",
	})
{
}

/*****************************************************************************/
bool BuildPaths::initialize()
{
	chalet_assert(!m_initialized, "BuildPaths::initialize called twice.");

	const auto& outputDirectory = m_state.inputs.outputDirectory();
	if (!Commands::pathExists(outputDirectory))
	{
		Commands::makeDirectory(outputDirectory);
	}

	const auto& buildConfig = m_state.info.buildConfiguration();
	const auto& toolchainPreference = m_state.inputs.toolchainPreferenceName();

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
		const auto& arch = m_state.info.targetArchitectureString();
		m_buildOutputDir = fmt::format("{}/{}_{}", outputDirectory, arch, buildConfig);
	}
	else // BuildPathStyle::TargetTriple
	{
		const auto& arch = m_state.inputs.getArchWithOptionsAsString(m_state.info.targetArchitectureTriple());
		m_buildOutputDir = fmt::format("{}/{}_{}", outputDirectory, arch, buildConfig);
	}

	m_intermediateDir = fmt::format("{}/int", outputDirectory);

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

	SourceGroup files = getFiles(inProject);

	for (auto& file : files.list)
	{
		auto ext = String::getPathSuffix(file);
		List::addIfDoesNotExist(m_allFileExtensions, std::move(ext));
	}

	List::sort(m_allFileExtensions);

	m_fileList.emplace(inProject.name(), std::make_unique<SourceGroup>(std::move(files)));
}

/*****************************************************************************/
const std::string& BuildPaths::homeDirectory() const noexcept
{
	return m_state.inputs.homeDirectory();
}

/*****************************************************************************/
const std::string& BuildPaths::rootDirectory() const noexcept
{
	return m_state.inputs.rootDirectory();
}

/*****************************************************************************/
const std::string& BuildPaths::outputDirectory() const noexcept
{
	return m_state.inputs.outputDirectory();
}

const std::string& BuildPaths::buildOutputDir() const
{
	chalet_assert(!m_buildOutputDir.empty(), "BuildPaths::buildOutputDir() called before BuildPaths::setBuildDirectoriesBasedOnProjectKind().");
	return m_buildOutputDir;
}

const std::string& BuildPaths::objDir() const
{
	chalet_assert(!m_objDir.empty(), "BuildPaths::objDir() called before BuildPaths::setBuildDirectoriesBasedOnProjectKind().");
	return m_objDir;
}

const std::string& BuildPaths::depDir() const
{
	chalet_assert(!m_depDir.empty(), "BuildPaths::depDir() called before BuildPaths::setBuildDirectoriesBasedOnProjectKind().");
	return m_depDir;
}

const std::string& BuildPaths::asmDir() const
{
	chalet_assert(!m_asmDir.empty(), "BuildPaths::asmDir() called before BuildPaths::setBuildDirectoriesBasedOnProjectKind().");
	return m_asmDir;
}

std::string BuildPaths::intermediateDir(const SourceTarget& inProject) const
{
	chalet_assert(!m_intermediateDir.empty(), "BuildPaths::intermediateDir() called before BuildPaths::setBuildDirectoriesBasedOnProjectKind().");
	return fmt::format("{}.{}", m_intermediateDir, inProject.buildSuffix());
}

/*****************************************************************************/
StringList BuildPaths::buildDirectories() const
{
	const auto& buildDir = buildOutputDir();
	return {
		fmt::format("{}/obj", buildDir),
		fmt::format("{}/asm", buildDir),
		fmt::format("{}/obj.shared", buildDir),
		fmt::format("{}/asm.shared", buildDir),
	};
}

/*****************************************************************************/
const StringList& BuildPaths::allFileExtensions() const noexcept
{
	return m_allFileExtensions;
}
StringList BuildPaths::objectiveCxxExtensions() const noexcept
{
	return List::combine(m_objectiveCExts, m_objectiveCppExts);
}
const StringList& BuildPaths::resourceExtensions() const noexcept
{
	return m_resourceExts;
}
const std::string& BuildPaths::cxxExtension() const
{
	return m_cxxExtension;
}

/*****************************************************************************/
void BuildPaths::setBuildDirectoriesBasedOnProjectKind(const SourceTarget& inProject)
{
	m_objDir = fmt::format("{}/obj.{}", m_buildOutputDir, inProject.buildSuffix());
	// m_depDir = fmt::format("{}/dep.{}", m_buildOutputDir, inProject.buildSuffix());
	m_asmDir = fmt::format("{}/asm.{}", m_buildOutputDir, inProject.buildSuffix());

	m_depDir = m_objDir;
}

/*****************************************************************************/
Unique<SourceOutputs> BuildPaths::getOutputs(const SourceTarget& inProject, StringList& outFileCache, const bool inDumpAssembly)
{
	auto ret = std::make_unique<SourceOutputs>();

	setBuildDirectoriesBasedOnProjectKind(inProject);

	chalet_assert(m_fileList.find(inProject.name()) != m_fileList.end(), "");

	SourceGroup files = *m_fileList.at(inProject.name());
	SourceGroup directories = getDirectories(inProject);

	for (const auto& file : files.list)
	{
		auto ext = String::getPathSuffix(file);
		List::addIfDoesNotExist(ret->fileExtensions, std::move(ext));
	}

	// inProject.isSharedLibrary() ? m_fileListCacheShared : m_fileListCache

	const bool isNotMsvc = !m_state.environment->isMsvc();
	ret->objectListLinker = getObjectFilesList(files.list, inProject);
	files.list = String::excludeIf(outFileCache, files.list);
	ret->groups = getSourceFileGroupList(std::move(files), inProject, outFileCache, inDumpAssembly);

	StringList objSubDirs = getOutputDirectoryList(directories, objDir());
	// StringList depSubDirs = getOutputDirectoryList(directories, depDir());
	StringList depSubDirs;

	StringList asmSubDirs;
	if (inDumpAssembly)
	{
		asmSubDirs = getOutputDirectoryList(directories, asmDir());

		if (isNotMsvc)
			ret->directories.reserve(4 + objSubDirs.size() + depSubDirs.size() + asmSubDirs.size());
		else
			ret->directories.reserve(4 + objSubDirs.size() + asmSubDirs.size());
	}
	else
	{
		if (isNotMsvc)
			ret->directories.reserve(3 + objSubDirs.size() + depSubDirs.size());
		else
			ret->directories.reserve(3 + objSubDirs.size());
	}

	ret->directories.push_back(m_buildOutputDir);
	ret->directories.push_back(objDir());

	ret->directories.push_back(intermediateDir(inProject));

	ret->directories.insert(ret->directories.end(), objSubDirs.begin(), objSubDirs.end());

	if (isNotMsvc)
	{
		ret->directories.push_back(depDir());
		ret->directories.insert(ret->directories.end(), depSubDirs.begin(), depSubDirs.end());
	}

	if (inDumpAssembly)
	{
		ret->directories.push_back(asmDir());
		ret->directories.insert(ret->directories.end(), asmSubDirs.begin(), asmSubDirs.end());
	}

	ret->target = getTargetFilename(inProject);

	return ret;
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

	if (inProject.unixSharedLibraryNamingConvention())
	{
		base = String::getPathFolderBaseName(inProject.outputFile());
	}
	else
	{
		base = name;
	}

	return fmt::format("{}/{}", buildOutputDir(), base);
}

/*****************************************************************************/
std::string BuildPaths::getPrecompiledHeaderTarget(const SourceTarget& inProject) const
{
	std::string ret;
	if (inProject.usesPrecompiledHeader())
	{
		std::string ext;
		if (m_state.environment->isClangOrMsvc() || m_state.environment->isIntelClassic())
			ext = "pch";
		// else if (m_state.environment->isIntelClassic())
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
	if (inProject.usesPrecompiledHeader())
	{
		const auto& pch = inProject.precompiledHeader();
		ret = fmt::format("{}/{}", objDir(), pch);
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getWindowsManifestFilename(const SourceTarget& inProject) const
{
	if (!inProject.isStaticLibrary() && inProject.windowsApplicationManifestGenerationEnabled())
	{
		if (inProject.windowsApplicationManifest().empty())
		{
			auto outputFile = inProject.outputFileNoPrefix();

			// https://docs.microsoft.com/en-us/windows/win32/sbscs/application-manifests#file-name-syntax
			return fmt::format("{}/{}.manifest", intermediateDir(inProject), outputFile);

			// return fmt::format("{}/default.manifest", intermediateDir());
		}
		else
		{
			return inProject.windowsApplicationManifest();
		}
	}

	return std::string();
}

/*****************************************************************************/
std::string BuildPaths::getWindowsManifestResourceFilename(const SourceTarget& inProject) const
{
	if (!inProject.isStaticLibrary() && inProject.windowsApplicationManifestGenerationEnabled())
	{
		/*if (inProject.windowsApplicationManifest().empty())
		{
			return fmt::format("{}/default.manifest.rc", intermediateDir());
		}
		else*/
		{
			const auto& name = inProject.name();
			return fmt::format("{}/manifest.rc", intermediateDir(inProject), name);
		}
	}

	return std::string();
}

/*****************************************************************************/
std::string BuildPaths::getWindowsIconResourceFilename(const SourceTarget& inProject) const
{
	if (inProject.isExecutable() && !inProject.windowsApplicationIcon().empty())
	{
		const auto& name = inProject.name();

		return fmt::format("{}/icon.rc", intermediateDir(inProject), name);
	}

	return std::string();
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
SourceFileGroupList BuildPaths::getSourceFileGroupList(SourceGroup&& inFiles, const SourceTarget& inProject, StringList& outFileCache, const bool inDumpAssembly)
{
	SourceFileGroupList ret;

	const bool isModule = inProject.cppModules();
	const bool sharesSourceFiles = inProject.sharesSourceFiles();

	for (auto& file : inFiles.list)
	{
		if (file.empty())
			continue;

		if (sharesSourceFiles)
			outFileCache.push_back(file);

		SourceType type = getSourceType(file);

		if (type == SourceType::Unknown)
			continue;

		if (!m_state.toolchain.canCompilerWindowsResources() && type == SourceType::WindowsResource)
			continue;

		auto group = std::make_unique<SourceFileGroup>();
		group->type = type;
		group->objectFile = getObjectFile(file);

		if (type == SourceType::CPlusPlus && isModule)
			group->dependencyFile = m_state.environment->getModuleDirectivesDependencyFile(file);
		else
			group->dependencyFile = m_state.environment->getDependencyFile(file);

		group->sourceFile = std::move(file);

		ret.push_back(std::move(group));
	}

	// don't do this for the pch
	if (inDumpAssembly)
	{
		for (auto& group : ret)
		{
			group->otherFile = getAssemblyFile(group->sourceFile);
		}
	}

	// add the pch
	if (!inFiles.pch.empty())
	{
		auto group = std::make_unique<SourceFileGroup>();

		group->type = SourceType::CxxPrecompiledHeader;
		group->objectFile = getPrecompiledHeaderTarget(inProject);
		group->dependencyFile = m_state.environment->getDependencyFile(inFiles.pch);
		group->sourceFile = std::move(inFiles.pch);

		ret.push_back(std::move(group));
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getObjectFile(const std::string& inSource) const
{
	if (String::endsWith(m_resourceExts, inSource))
	{
		if (m_state.toolchain.canCompilerWindowsResources())
			return fmt::format("{}/{}.res", objDir(), inSource);
	}
	else
	{
		return m_state.environment->getObjectFile(inSource);
	}

	return std::string();
}

/*****************************************************************************/
std::string BuildPaths::getAssemblyFile(const std::string& inSource) const
{
	if (String::endsWith(m_resourceExts, inSource))
	{
		return std::string();
	}
	else
	{
		return m_state.environment->getAssemblyFile(inSource);
	}
}

/*****************************************************************************/
SourceType BuildPaths::getSourceType(const std::string& inSource) const
{
	const auto ext = String::getPathSuffix(inSource);
	if (!ext.empty())
	{
		if (String::equals(m_cExts, ext))
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
		else
		{
			if (m_cxxExtension.empty())
				m_cxxExtension = ext;

			return SourceType::CPlusPlus;
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
		auto outFile = getObjectFile(file);
		if (!outFile.empty())
			ret.emplace_back(std::move(outFile));
	}

#if defined(CHALET_WIN32)
	if (m_state.environment->isMsvc())
	{
		if (inProject.usesPrecompiledHeader())
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

	// StringList extensions = List::combine(m_cExts, m_cppExts, m_cppModuleExts, m_resourceExts, m_objectiveCExts, m_objectiveCppExts);

	const auto& files = inProject.files();
	auto& pch = inProject.precompiledHeader();
	bool usesPch = inProject.usesPrecompiledHeader();
	StringList fileList;

	for (auto& file : files)
	{
		if (usesPch && String::equals(pch, file))
		{
			Diagnostic::warn("Precompiled header explicitly included in 'files': {} (ignored)", file);
			continue;
		}

		/*if (!String::endsWith(extensions, file))
		{
			Diagnostic::warn("File type in 'files' is not required or supported: {} (ignored)", file);
			continue;
		}*/

		if (!Commands::pathExists(file))
		{
			Diagnostic::warn("File not found: {}", file);
			continue;
		}

		if (Commands::pathIsFile(file))
		{
			List::addIfDoesNotExist(fileList, file);
		}
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

/*****************************************************************************/
StringList BuildPaths::getDirectoryList(const SourceTarget& inProject) const
{
	CHALET_TRY
	{
		StringList ret;

		if (inProject.usesPrecompiledHeader())
		{
			if (Commands::pathExists(inProject.precompiledHeader()))
			{
				std::string outPath = String::getPathFolder(inProject.precompiledHeader());
				Path::sanitize(outPath, true);

#if defined(CHALET_MACOS)
				if (!m_state.inputs.universalArches().empty())
				{
					for (auto& arch : m_state.inputs.universalArches())
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
		for (auto& file : files)
		{
			if (!Commands::pathExists(file))
				continue;

			std::string outPath = String::getPathFolder(file);
			Path::sanitize(outPath, true);

			List::addIfDoesNotExist(ret, std::move(outPath));
		}

		List::addIfDoesNotExist(ret, intermediateDir(inProject));

		return ret;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR(err.what());
		return StringList();
	}
}

/*****************************************************************************/
BuildPaths::SourceGroup BuildPaths::getFiles(const SourceTarget& inProject) const
{
	SourceGroup ret;
	ret.list = getFileList(inProject);
	ret.pch = inProject.precompiledHeader();

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
