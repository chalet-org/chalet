/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/BinaryDependency/BinaryDependencyMap.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Bundler/BinaryDependency/DependencyWalker.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
BinaryDependencyMap::BinaryDependencyMap(const BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
BinaryDependencyMap::InnerMap::const_iterator BinaryDependencyMap::begin() const
{
	return m_map.begin();
}

BinaryDependencyMap::InnerMap::const_iterator BinaryDependencyMap::end() const
{
	return m_map.end();
}

/*****************************************************************************/
void BinaryDependencyMap::setIncludeWinUCRT(const bool inValue)
{
	m_includeWinUCRT = inValue;
}

/*****************************************************************************/
void BinaryDependencyMap::clearSearchDirs() noexcept
{
	m_searchDirs.clear();
}

/*****************************************************************************/
void BinaryDependencyMap::addSearchDirsFromList(const StringList& inList)
{
	for (auto& item : inList)
	{
		if (!Files::pathExists(item))
			continue;

		List::addIfDoesNotExist(m_searchDirs, item);
	}
}

/*****************************************************************************/
void BinaryDependencyMap::log() const
{
	for (auto& [file, dependencies] : m_map)
	{
		LOG(file);
		for (auto& dep : dependencies)
		{
			LOG("    ", dep);
		}
	}
	if (!m_map.empty())
	{
		LOG("");
	}
}

/*****************************************************************************/
void BinaryDependencyMap::populateToList(std::map<std::string, std::string>& outMap, const StringList& inExclusions) const
{
	for (auto& [item, mapping] : m_list)
	{
		if (List::contains(inExclusions, item))
			continue;

		if (outMap.find(item) == outMap.end())
			outMap.emplace(item, mapping);
	}
}

/*****************************************************************************/
bool BinaryDependencyMap::gatherFromList(const std::map<std::string, std::string>& inMap, i32 levels)
{
	m_map.clear();
	m_list.clear();

	if (levels > 0)
	{
		for (auto& [outputFilePath, mapping] : inMap)
		{
			if (!gatherDependenciesOf(outputFilePath, mapping, levels))
				return false;
		}
	}

	for (auto& [file, dependencies] : m_map)
	{
		for (auto it = m_notCopied.begin(); it != m_notCopied.end();)
		{
			if (String::endsWith(*it, file))
			{
				it = m_notCopied.erase(it);
				continue;
			}
			else
			{
				++it;
			}
		}
	}

	return true;
}

/*****************************************************************************/
const StringList& BinaryDependencyMap::notCopied() const noexcept
{
	return m_notCopied;
}

/*****************************************************************************/
bool BinaryDependencyMap::gatherDependenciesOf(const std::string& inPath, const std::string& inMapping, i32 levels)
{
#if defined(CHALET_MACOS)
	auto framework = Files::getPlatformFrameworkExtension();
	if (String::endsWith(framework, inPath) || String::startsWith("/usr/lib/", inPath))
		return true;
#endif

	if (m_map.find(inPath) != m_map.end())
		return true;

	StringList dependencies;
	if (!getExecutableDependencies(inPath, dependencies))
	{
		Diagnostic::error("Dependencies not found for file: '{}'", inPath);
		return false;
	}

	--levels;

	bool ignoreApiSet = !m_includeWinUCRT;
	for (auto it = dependencies.begin(); it != dependencies.end();)
	{
		if (!resolveDependencyPath(*it, inPath, ignoreApiSet))
		{
			m_notCopied.emplace_back(*it);
			it = dependencies.erase(it);
			continue;
		}
		else
		{
			if (m_list.find(*it) == m_list.end())
				m_list.emplace(*it, inMapping);

			if (levels > 0)
			{
				if (!gatherDependenciesOf(*it, inMapping, levels))
					return false;
			}
			++it;
		}
	}

	m_map.emplace(inPath, std::move(dependencies));
	return true;
}

/*****************************************************************************/
bool BinaryDependencyMap::resolveDependencyPath(std::string& outDep, const std::string& inParentDep, const bool inIgnoreApiSet)
{
	const auto filename = String::getPathFilename(outDep);
	if (outDep.empty() || filename.empty())
		return false;

#if defined(CHALET_WIN32)
	if (String::startsWith("api-ms-win-", filename))
	{
		auto ucrtDir = Environment::getString("UniversalCRTSdkDir");
		auto arch = Environment::getString("VSCMD_ARG_TGT_ARCH");
		if (!ucrtDir.empty() && !arch.empty())
		{
			auto ucrtVersion = Environment::getString("UCRTVersion");

			if (ucrtDir.back() == '\\')
				ucrtDir.pop_back();

			auto res = fmt::format("{}/Redist/{}/ucrt/DLLs/{}/{}", ucrtDir, ucrtVersion, arch, filename);
			Path::toUnix(res);
			if (!ucrtVersion.empty() && Files::pathExists(res))
			{
				outDep = std::move(res);
				return true;
			}
			else
			{
				res = fmt::format("{}/Redist/ucrt/DLLs/{}/{}", ucrtDir, arch, filename);
				Path::toUnix(res);
				if (Files::pathExists(res))
				{
					outDep = std::move(res);
					return true;
				}
			}
		}

		// Note: If one of these dlls can't be resolved, it's probably an API set loader, so we don't care about it
		// Example: api-ms-win-shcore-scaling-l1-1-1.dll -> Shcore.dll
		//
		// Info:
		//   https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-apisets
		//   https://learn.microsoft.com/en-us/windows/win32/apiindex/api-set-loader-operation
		if (inIgnoreApiSet)
		{
			outDep = std::string();
		}
		return false;
	}
#else
	UNUSED(inIgnoreApiSet);
#endif

	if (Files::pathExists(outDep))
		return true;

	std::string resolved;
	for (auto& dir : m_searchDirs)
	{
		resolved = fmt::format("{}/{}", dir, filename);
		if (Files::pathExists(resolved))
		{
			outDep = std::move(resolved);
			return true;
		}

		resolved.clear();
	}

	// Fixes a problem resolving libgcc_s.1.1.dylib from homebrew gcc on mac
	resolved = fmt::format("{}/{}", String::getPathFolder(inParentDep), String::getPathFilename(outDep));
	if (Files::pathExists(resolved))
	{
		outDep = std::move(resolved);
		return true;
	}

	resolved = Files::which(filename);
	if (resolved.empty())
		return false;

	outDep = std::move(resolved);
	return true;
}

/*****************************************************************************/
bool BinaryDependencyMap::getExecutableDependencies(const std::string& inPath, StringList& outList, StringList* outNotFound)
{
	if (m_state.environment->isWindowsTarget())
	{
		DependencyWalker depsWalker;
		if (!depsWalker.read(inPath, outList, outNotFound, m_includeWinUCRT))
		{
			Diagnostic::error("Dependencies for file '{}' could not be read.", inPath);
			return false;
		}

		return true;
	}

#if defined(CHALET_MACOS)
	const auto& otool = m_state.tools.otool();
	if (otool.empty())
	{
		Diagnostic::error("Dependencies for file '{}' could not be read. 'otool' was not found in cache.", inPath);
		return false;
	}
#else
	const auto& ldd = m_state.tools.ldd();
	if (ldd.empty())
	{
		Diagnostic::error("Dependencies for file '{}' could not be read. 'ldd' was not found in cache.", inPath);
		return false;
	}
#endif

	CHALET_TRY
	{
#if defined(CHALET_MACOS)
		StringList cmd{ otool, "-L", inPath };
		auto dylib = Files::getPlatformSharedLibraryExtension();
		auto framework = Files::getPlatformFrameworkExtension();
#else
		// This block detects the dependencies of each target and adds them to a list
		// The list resolves each path, favoring the paths supplied by chalet.json
		// Note: this doesn't seem to work in standalone builds of GCC (tested 7.3.0)
		//   but works fine w/ MSYS2
		StringList cmd{ ldd, inPath };
#endif
		std::string targetDeps = Process::runOutput(cmd);

		std::istringstream input(targetDeps);

#if defined(CHALET_LINUX)
		auto librarySearchPathString = Environment::getString(Environment::getLibraryPathKey());
		auto librarySearchPaths = String::split(librarySearchPathString, ':');
#endif

		std::string line;
		auto lineEnd = input.widen('\n');
		while (std::getline(input, line, lineEnd))
		{
			size_t beg = 0;

			if (String::startsWith("Archive", line))
				break;

			if (String::startsWith(inPath, line))
				continue;

			while (line[beg] == '\t' || line[beg] == ' ')
				beg++;

#if defined(CHALET_MACOS)
			size_t end = line.find(dylib);
			if (end == std::string::npos)
			{
				end = line.find(framework);
				if (end == std::string::npos)
					continue;
				else
					end += 10;
			}
			else
			{
				end += 6;
			}
#else
			size_t end = line.find("=>");
			if (end != std::string::npos && end > 0)
				end--;
#endif

			std::string dependencyFile;
			std::string dependency = line.substr(beg, end - beg);
#if defined(CHALET_MACOS)
			if (String::startsWith("/System/Library/Frameworks/", dependency))
				continue;

			// rpath, etc
			// We just want the main filename, and will try to resolve the path later
			//
			if (String::startsWith('@', dependency) || String::contains(framework, dependency))
			{
				auto lastSlash = dependency.find_last_of('/');
				if (lastSlash != std::string::npos)
					dependency = dependency.substr(lastSlash + 1);
			}
			dependencyFile = String::getPathFilename(dependency);
#elif defined(CHALET_LINUX)
			dependencyFile = String::getPathFilename(dependency);
			{
				auto space = dependencyFile.find(' ');
				if (space != std::string::npos)
					dependencyFile = dependencyFile.substr(0, space);
			}

			dependency.clear();

			// We only want our search paths
			for (auto& path : librarySearchPaths)
			{
				auto resolved = fmt::format("{}/{}", path, dependencyFile);
				if (Files::pathExists(resolved))
				{
					dependency = std::move(resolved);
					break;
				}
			}
#else // Windows
			dependencyFile = String::getPathFilename(dependency);
			dependency = Files::which(dependencyFile);
#endif

			if (dependency.empty())
			{
				if (outNotFound != nullptr && !dependencyFile.empty())
				{
					outNotFound->emplace_back(std::move(dependencyFile));
				}
				continue;
			}

#if defined(CHALET_LINUX) || defined(CHALET_MACOS)
			if (String::startsWith("/lib/", dependency))
				continue;
#endif

			List::addIfDoesNotExist(outList, std::move(dependency));
		}

		return true;
	}
	CHALET_CATCH(const std::runtime_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what());
		return false;
	}
}

}
