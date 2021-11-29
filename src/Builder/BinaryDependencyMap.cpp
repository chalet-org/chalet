/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BinaryDependencyMap.hpp"

#include "State/AncillaryTools.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/DependencyWalker.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
BinaryDependencyMap::BinaryDependencyMap(const AncillaryTools& inTools) :
	m_tools(inTools)
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
void BinaryDependencyMap::addExcludesFromList(const StringList& inList)
{
	m_excludes.clear();
	for (auto& item : inList)
	{
		if (!Commands::pathExists(item))
			continue;

		List::addIfDoesNotExist(m_excludes, item);
	}
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
		if (!Commands::pathExists(item))
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
	LOG("");
}

/*****************************************************************************/
void BinaryDependencyMap::populateToList(StringList& outList, const StringList& inExclusions) const
{
	for (auto& item : m_list)
	{
		if (List::contains(inExclusions, item))
			continue;

		List::addIfDoesNotExist(outList, item);
	}
}

/*****************************************************************************/
bool BinaryDependencyMap::gatherFromList(const StringList& inList, int levels)
{
	m_map.clear();
	m_list.clear();

	if (levels > 0)
	{
		for (auto& outputFilePath : inList)
		{
			if (!gatherDependenciesOf(outputFilePath, levels))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool BinaryDependencyMap::gatherDependenciesOf(const std::string& inPath, int levels)
{
	if (m_map.find(inPath) != m_map.end())
		return true;

#if defined(CHALET_MACOS)
	if (String::endsWith(".framework", inPath) || String::startsWith("/usr/lib/", inPath))
		return true;
#endif

	StringList dependencies;
	if (!getExecutableDependencies(inPath, dependencies))
	{
		Diagnostic::error("Dependencies not found for file: '{}'", inPath);
		return false;
	}

	--levels;

	for (auto it = dependencies.begin(); it != dependencies.end();)
	{
		if (!resolveDependencyPath(*it))
		{
			it = dependencies.erase(it);
			continue;
		}
		else
		{
			List::addIfDoesNotExist(m_list, *it);
			if (levels > 0)
			{
				if (!gatherDependenciesOf(*it, levels))
					return false;
			}
			++it;
		}
	}

	m_map.emplace(inPath, std::move(dependencies));
	return true;
}

/*****************************************************************************/
bool BinaryDependencyMap::resolveDependencyPath(std::string& outDep) const
{
	const auto filename = String::getPathFilename(outDep);
	if (outDep.empty()
		|| List::contains(m_excludes, outDep)
		|| List::contains(m_excludes, filename))
		return false;

	if (Commands::pathExists(outDep))
		return true;

	std::string resolved;
	for (auto& dir : m_searchDirs)
	{
		resolved = fmt::format("{}/{}", dir, filename);
		if (Commands::pathExists(resolved))
		{
			outDep = std::move(resolved);
			return true;
		}

		resolved.clear();
	}

	resolved = Commands::which(filename);
	if (resolved.empty())
	{
		// Diagnostic::warn("Dependency not copied (not found in path): '{}'", outDep);
		// return false;
		// We probably don't care about them anyway
		return false;
	}

	outDep = std::move(resolved);
	return true;
}

/*****************************************************************************/
bool BinaryDependencyMap::getExecutableDependencies(const std::string& inPath, StringList& outList) const
{
#if defined(CHALET_WIN32)
	DependencyWalker depsWalker;
	if (!depsWalker.read(inPath, outList))
	{
		Diagnostic::error("Dependencies for file '{}' could not be read.", inPath);
		return false;
	}

	return true;
#else
	#if defined(CHALET_MACOS)
	const auto& otool = m_tools.otool();
	if (otool.empty())
	{
		Diagnostic::error("Dependencies for file '{}' could not be read. 'otool' was not found in cache.", inPath);
		return false;
	}
	#else
	const auto& ldd = m_tools.ldd();
	if (ldd.empty())
	{
		Diagnostic::error("Dependencies for file '{}' could not be read. 'ldd' was not found in cache.", inPath);
		return false;
	}
	#endif

	CHALET_TRY
	{
		StringList cmd;
	#if defined(CHALET_MACOS)
		cmd = { otool, "-L", inPath };
	#else
		// This block detects the dependencies of each target and adds them to a list
		// The list resolves each path, favoring the paths supplied by chalet.json
		// Note: this doesn't seem to work in standalone builds of GCC (tested 7.3.0)
		//   but works fine w/ MSYS2
		cmd = { ldd, inPath };
	#endif
		std::string targetDeps = Commands::subprocessOutput(cmd);

		std::string line;
		std::istringstream stream(targetDeps);

		while (std::getline(stream, line))
		{
			std::size_t beg = 0;

			if (String::startsWith("Archive", line))
				break;

			if (String::startsWith(inPath, line))
				continue;

			while (line[beg] == '\t' || line[beg] == ' ')
				beg++;

	#if defined(CHALET_MACOS)
			std::size_t end = line.find(".dylib");
			if (end == std::string::npos)
			{
				end = line.find(".framework");
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
			std::size_t end = line.find("=>");
			if (end != std::string::npos && end > 0)
				end--;
	#endif

			std::string dependency = line.substr(beg, end - beg);
	#if defined(CHALET_MACOS)
			if (String::startsWith("/System/Library/Frameworks/", dependency))
				continue;

			// rpath, etc
			// We just want the main filename, and will try to resolve the path later
			//
			if (String::startsWith('@', dependency) || String::contains(".framework", dependency))
			{
				auto lastSlash = dependency.find_last_of('/');
				if (lastSlash != std::string::npos)
					dependency = dependency.substr(lastSlash + 1);
			}
	#else
			dependency = Commands::which(String::getPathFilename(dependency));
	#endif

			if (dependency.empty())
				continue;

	#if defined(CHALET_LINUX)
			if (String::startsWith("/usr/lib/", dependency))
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
#endif
}

}
