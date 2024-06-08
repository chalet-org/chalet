/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildPaths.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Dependency/IExternalDependency.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

#include "State/Dependency/LocalDependency.hpp"
#include "State/Dependency/ScriptDependency.hpp"

namespace chalet
{
BuildPaths::BuildPaths(const BuildState& inState) :
	m_state(inState),
	m_resourceExts({
		"rc",
		"RC",
	}),
	m_objectiveCExts({
		"m",
		"M",
	}),
	m_objectiveCppExt("mm")
{
}

/*****************************************************************************/
bool BuildPaths::initialize()
{
	chalet_assert(!m_initialized, "BuildPaths::initialize called twice.");

	m_parentCwd = Environment::getString("__CHALET_PARENT_CWD");

	auto outputDirectory = m_state.inputs.outputDirectory();
	bool isChaletTarget = m_state.isSubChaletTarget();
	if (!Files::pathExists(outputDirectory))
	{
		Files::makeDirectory(outputDirectory);
	}

	const auto& buildConfig = m_state.info.buildConfiguration();
	const auto& toolchainPreference = m_state.inputs.toolchainPreferenceName();
	const auto& arch = m_state.info.targetArchitectureString();

	if (!isChaletTarget)
	{
		auto style = m_state.toolchain.buildPathStyle();
		if (style == BuildPathStyle::ToolchainName && !toolchainPreference.empty())
		{
			if (m_state.inputs.isMultiArchToolchainPreset())
				m_buildOutputDir = fmt::format("{}/{}-{}_{}", outputDirectory, arch, toolchainPreference, buildConfig);
			else
				m_buildOutputDir = fmt::format("{}/{}_{}", outputDirectory, toolchainPreference, buildConfig);
		}
		else if (style == BuildPathStyle::Configuration)
		{
			m_buildOutputDir = fmt::format("{}/{}", outputDirectory, buildConfig);
		}
		else if (style == BuildPathStyle::ArchConfiguration)
		{
			m_buildOutputDir = fmt::format("{}/{}_{}", outputDirectory, arch, buildConfig);
		}
		else // BuildPathStyle::TargetTriple
		{
			const auto& arch2 = m_state.inputs.getArchWithOptionsAsString(m_state.info.targetArchitectureTriple());
			m_buildOutputDir = fmt::format("{}/{}_{}", outputDirectory, arch2, buildConfig);
		}
	}
	else
	{
		m_buildOutputDir = outputDirectory;
	}

	m_externalBuildDir = fmt::format("{}/ext", m_buildOutputDir, m_state.inputs.externalDirectory());

	m_initialized = true;

	return true;
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
	chalet_assert(!m_buildOutputDir.empty(), "BuildPaths::buildOutputDir() called before BuildPaths::initialize().");
	return m_buildOutputDir;
}

const std::string& BuildPaths::externalBuildDir() const
{
	chalet_assert(!m_externalBuildDir.empty(), "BuildPaths::externalBuildDir() called before BuildPaths::initialize().");
	return m_externalBuildDir;
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

/*****************************************************************************/
std::string BuildPaths::intermediateDir(const SourceTarget& inProject) const
{
	return fmt::format("{}/{}", m_intermediateDir, inProject.buildSuffix());
}
std::string BuildPaths::intermediateIncludeDir(const SourceTarget& inProject) const
{
	auto intDir = intermediateDir(inProject);
	return fmt::format("{}/include", intDir);
}

/*****************************************************************************/
std::string BuildPaths::bundleObjDir(const std::string& inName) const
{
	return fmt::format("{}/dist.{}", buildOutputDir(), inName);
}

/*****************************************************************************/
std::string BuildPaths::currentCompileCommands() const
{
	return fmt::format("{}/compile_commands.json", outputDirectory());
}

/*****************************************************************************/
StringList BuildPaths::getBuildDirectories(const SourceTarget& inProject) const
{
	const auto& buildDir = buildOutputDir();
	StringList ret{
		fmt::format("{}/obj.{}", buildDir, inProject.buildSuffix()),
		fmt::format("{}/asm.{}", buildDir, inProject.buildSuffix()),
		fmt::format("{}/int.{}", buildDir, inProject.buildSuffix()),
	};

#if defined(CHALET_MACOS)
	// Xcode build paths - if the strategy is xcodebuild,
	//   or if the project was exported, followed by a 'chalet clean' command
	{
		ret.emplace_back(fmt::format("{}/obj.{}", buildDir, inProject.name()));
		ret.emplace_back(fmt::format("{}/obj.{}-normal", buildDir, inProject.name()));
		ret.emplace_back(fmt::format("{}/EagerLinkingTBDs", buildDir));
		ret.emplace_back(fmt::format("{}/SharedPrecompiledHeaders", buildDir));
		ret.emplace_back(fmt::format("{}/XCBuildData", buildDir));
		ret.emplace_back(fmt::format("{}/{}.dSYM", buildDir, inProject.name()));
	}
#endif
	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getExternalDir(const std::string& inName) const
{
	for (auto& dep : m_state.externalDependencies)
	{
		if (String::equals(dep->name(), inName))
		{
			if (dep->isGit())
			{
				return fmt::format("{}/{}", m_state.inputs.externalDirectory(), dep->name());
			}
			else if (dep->isScript())
			{
				const auto& scriptDep = static_cast<const ScriptDependency&>(*dep);
				return scriptDep.file();
			}
			else if (dep->isLocal())
			{
				const auto& localDep = static_cast<const LocalDependency&>(*dep);
				return localDep.path();
			}
		}
	}

	return std::string();
}

/*****************************************************************************/
std::string BuildPaths::getExternalBuildDir(const std::string& inName) const
{
	for (auto& target : m_state.targets)
	{
		if (!target->isCMake() && !target->isSubChalet())
			continue;

		if (String::equals(target->name(), inName))
		{
			return fmt::format("{}.{}", externalBuildDir(), inName);
		}
	}

	return std::string();
}

/*****************************************************************************/
const StringList& BuildPaths::windowsResourceExtensions() const noexcept
{
	return m_resourceExts;
}
const StringList& BuildPaths::objectiveCExtensions() const noexcept
{
	return m_objectiveCExts;
}
const std::string& BuildPaths::objectiveCppExtension() const
{
	return m_objectiveCppExt;
}

/*****************************************************************************/
void BuildPaths::setBuildDirectoriesBasedOnProjectKind(const SourceTarget& inProject)
{
	m_objDir = fmt::format("{}/obj.{}", m_buildOutputDir, inProject.buildSuffix());
	m_asmDir = fmt::format("{}/asm.{}", m_buildOutputDir, inProject.buildSuffix());
	m_intermediateDir = fmt::format("{}/int", m_buildOutputDir);

	m_intermediateDirWithPathSep = intermediateDir(inProject) + '/';

	m_depDir = m_objDir;
}

/*****************************************************************************/
Unique<SourceOutputs> BuildPaths::getOutputs(const SourceTarget& inProject, StringList& outFileCache)
{
	auto ret = std::make_unique<SourceOutputs>();

	setBuildDirectoriesBasedOnProjectKind(inProject);

	auto files = getFiles(inProject);
	SourceGroup directories = getDirectories(inProject);

	// inProject.isSharedLibrary() ? m_fileListCacheShared : m_fileListCache

	const bool isNotMsvc = !m_state.environment->isMsvc();
	const bool dumpAssembly = m_state.info.dumpAssembly();

	ret->objectListLinker = getObjectFilesList(files->list, inProject);
	files->list = String::excludeIf(outFileCache, files->list);
	ret->groups = getSourceFileGroupList(*files, inProject, outFileCache);

	StringList objSubDirs = getOutputDirectoryList(directories, objDir());
	// StringList depSubDirs = getOutputDirectoryList(directories, depDir());
	StringList depSubDirs;

	StringList asmSubDirs;
	if (dumpAssembly)
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

	if (dumpAssembly)
	{
		ret->directories.push_back(asmDir());
		ret->directories.insert(ret->directories.end(), asmSubDirs.begin(), asmSubDirs.end());
	}

	// for (auto& dir : ret->directories)
	// {
	// 	LOG(dir);
	// }

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
std::string BuildPaths::getTargetFilename(const CMakeTarget& inProject) const
{
	auto filename = inProject.runExecutable();
	if (filename.empty())
		return std::string();

	// Ignore the extension and enforce the one from the environment
	//   If it was anything else, we wouldn't recognize it anyway
	//
	filename = String::getPathFolderBaseName(filename);
	filename += m_state.environment->getExecutableExtension();

	return fmt::format("{}/{}", inProject.targetFolder(), filename);
}

/*****************************************************************************/
std::string BuildPaths::getTargetBasename(const SourceTarget& inProject) const
{
	auto base = String::getPathFolderBaseName(inProject.outputFile());
	return fmt::format("{}/{}", buildOutputDir(), base);
}

/*****************************************************************************/
std::string BuildPaths::getExecutableTargetPath(const IBuildTarget& inTarget) const
{
	if (inTarget.isSources())
	{
		return getTargetFilename(static_cast<const SourceTarget&>(inTarget));
	}
	else if (inTarget.isCMake())
	{
		return getTargetFilename(static_cast<const CMakeTarget&>(inTarget));
	}

	return std::string();
}

/*****************************************************************************/
std::string BuildPaths::getPrecompiledHeaderTarget(const SourceTarget& inProject) const
{
	std::string ret;
	if (inProject.usesPrecompiledHeader())
	{
		auto base = getPrecompiledHeaderInclude(inProject);
		auto ext = m_state.environment->getPrecompiledHeaderExtension();
		ret = fmt::format("{}{}", base, ext);
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getPrecompiledHeaderObject(const std::string& inTarget) const
{
	if (m_state.environment->isMsvc())
	{
		auto base = String::getPathFolderBaseName(inTarget);
		return base + ".obj";
	}

	return inTarget;
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
	bool canUseManifest = inProject.isExecutable() || inProject.isSharedLibrary();
	if (canUseManifest && inProject.windowsApplicationManifestGenerationEnabled())
	{
		const auto& manifest = inProject.windowsApplicationManifest();
		if (!manifest.empty())
			return manifest;

		// https://docs.microsoft.com/en-us/windows/win32/sbscs/application-manifests#file-name-syntax
		return fmt::format("{}/{}.manifest", intermediateDir(inProject), inProject.outputFile());
	}

	return std::string();
}

/*****************************************************************************/
std::string BuildPaths::getWindowsManifestResourceFilename(const SourceTarget& inProject) const
{
	bool canUseManifest = inProject.isExecutable() || inProject.isSharedLibrary();
	if (canUseManifest && inProject.windowsApplicationManifestGenerationEnabled())
	{
		const auto& name = inProject.name();
		return fmt::format("{}/{}_manifest.rc", intermediateDir(inProject), name);
	}

	return std::string();
}

/*****************************************************************************/
std::string BuildPaths::getWindowsIconResourceFilename(const SourceTarget& inProject) const
{
	if (inProject.isExecutable() && !inProject.windowsApplicationIcon().empty())
	{
		const auto& name = inProject.name();
		return fmt::format("{}/{}_icon.rc", intermediateDir(inProject), name);
	}

	return std::string();
}

/*****************************************************************************/
std::string BuildPaths::getUnityBuildSourceFilename(const SourceTarget& inProject) const
{
	if (inProject.unityBuild())
	{
		const auto& name = inProject.name();
		return fmt::format("{}/{}_unity.cxx", intermediateDir(inProject), name);
	}

	return std::string();
}

/*****************************************************************************/
std::string BuildPaths::getNormalizedOutputPath(const std::string& inPath) const
{
	std::string ret = inPath;

	normalizedPath(ret);

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getNormalizedDirectoryPath(const std::string& inPath) const
{
	auto ret = String::getPathFolder(inPath);
	Path::toUnix(ret, true);

	normalizedPath(ret);

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getBuildOutputPath(std::string path) const
{
	if (!m_parentCwd.empty())
	{
		String::replaceAll(path, m_parentCwd, "");
		return path;
	}

	return path;
}

/*****************************************************************************/
// Note: this might seem confusing, but it's to take relative input paths and keep output paths inside
//   the build directory
//
void BuildPaths::normalizedPath(std::string& outPath) const
{
	if (String::startsWith(m_intermediateDirWithPathSep, outPath))
		outPath = outPath.substr(m_intermediateDirWithPathSep.size());

	String::replaceAll(outPath, "/../", "/p/");

	if (String::startsWith("../", outPath))
	{
		outPath = fmt::format("p{}", outPath.substr(2));
	}
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
SourceFileGroupList BuildPaths::getSourceFileGroupList(const SourceGroup& inFiles, const SourceTarget& inProject, StringList& outFileCache)
{
	SourceFileGroupList ret;

	const bool isModule = inProject.cppModules();

	auto makeGroup = [this, &isModule](SourceType type, const std::string& file) {
		auto group = std::make_unique<SourceFileGroup>();
		group->type = type;
		group->objectFile = getObjectFile(file);

		if (type == SourceType::CPlusPlus && isModule)
			group->dependencyFile = m_state.environment->getModuleDirectivesDependencyFile(file);
		else
			group->dependencyFile = m_state.environment->getDependencyFile(file);

		group->sourceFile = file;
		return group;
	};

	for (auto& file : inFiles.list)
	{
		if (file.empty())
			continue;

		outFileCache.push_back(file);

		SourceType type = getSourceType(file);

		if (type == SourceType::Unknown)
			continue;

		if (!m_state.toolchain.canCompileWindowsResources() && type == SourceType::WindowsResource)
			continue;

		ret.push_back(makeGroup(type, file));
	}

	// don't do this for the pch
	if (m_state.info.dumpAssembly())
	{
		for (auto& group : ret)
		{
			group->otherFile = getAssemblyFile(group->sourceFile);
		}
	}

	// add the pch
	if (!inFiles.pch.empty())
	{
		auto makePch = [this, &inProject](const std::string& file) {
			auto group = std::make_unique<SourceFileGroup>();

			group->type = SourceType::CxxPrecompiledHeader;
			group->objectFile = getPrecompiledHeaderTarget(inProject);
			group->dependencyFile = m_state.environment->getDependencyFile(file);
			group->sourceFile = file;
			group->otherFile = m_state.environment->getPrecompiledHeaderSourceFile(inProject);
			return group;
		};

		ret.push_back(makePch(inFiles.pch));
	}

	return ret;
}

/*****************************************************************************/
std::string BuildPaths::getObjectFile(const std::string& inSource) const
{
	if (String::endsWith(m_resourceExts, inSource))
	{
		if (m_state.toolchain.canCompileWindowsResources())
			return m_state.environment->getWindowsResourceObjectFile(inSource);
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
		if (String::equals('c', ext))
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
		else if (String::equals(m_objectiveCppExt, ext))
		{
			return SourceType::ObjectiveCPlusPlus;
		}
		else
		{
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
			ret.emplace_back(getPrecompiledHeaderObject(getPrecompiledHeaderTarget(inProject)));
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
	auto intDir = fmt::format("{}/int", buildOutputDir());
	std::for_each(ret.begin(), ret.end(), [&inFolder, &intDir](std::string& str) {
		if (String::startsWith(intDir, str))
			str = fmt::format("{}/int", inFolder); // obj.(name)/int
		else
			str = fmt::format("{}/{}", inFolder, str);
	});

	return ret;
}

/*****************************************************************************/
StringList BuildPaths::getFileList(const SourceTarget& inProject) const
{
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

		if (!Files::pathExists(file) || !Files::pathIsFile(file))
		{
			Diagnostic::warn("File not found: {}", file);
			continue;
		}

		fileList.emplace_back(file);
	}

	auto manifestResource = getWindowsManifestResourceFilename(inProject);
	if (!manifestResource.empty())
	{
		fileList.emplace_back(std::move(manifestResource));
	}

	auto iconResource = getWindowsIconResourceFilename(inProject);
	if (!iconResource.empty())
	{
		fileList.emplace_back(std::move(iconResource));
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
			if (Files::pathExists(inProject.precompiledHeader()))
			{
				auto outPath = getNormalizedDirectoryPath(inProject.precompiledHeader());

#if defined(CHALET_MACOS)
				if (!m_state.inputs.universalArches().empty())
				{
					for (auto& arch : m_state.inputs.universalArches())
					{
						ret.emplace_back(fmt::format("{}_{}", outPath, arch));
					}
				}
#endif
				ret.emplace_back(std::move(outPath));
			}
		}

		const auto& files = inProject.files();
		for (auto& file : files)
		{
			if (!Files::pathExists(file))
				continue;

			List::addIfDoesNotExist(ret, getNormalizedDirectoryPath(file));
		}

		return ret;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR(err.what());
		return StringList();
	}
}

/*****************************************************************************/
Ref<BuildPaths::SourceGroup> BuildPaths::getFiles(const SourceTarget& inProject) const
{
	auto ret = std::make_shared<SourceGroup>();
	ret->list = getFileList(inProject);
	ret->pch = inProject.precompiledHeader();

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
