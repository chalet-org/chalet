/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/WorkspaceEnvironment.hpp"

#include <thread>

#include "Libraries/FileSystem.hpp"

#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
WorkspaceEnvironment::WorkspaceEnvironment() :
	m_processorCount(std::thread::hardware_concurrency()),
	m_maxJobs(m_processorCount)
{
	// LOG("Processor count: ", m_processorCount);
}

/*****************************************************************************/
bool WorkspaceEnvironment::initialize(BuildPaths& inPaths)
{
	auto originalPathVar = Environment::getPath();
	std::string addedPath;
	for (auto& path : m_searchPaths)
	{
		if (String::contains(path + Path::getSeparator(), originalPathVar))
			continue;

		inPaths.replaceVariablesInPath(path);
		addedPath += path;
		addedPath += Path::getSeparator();
	}

	if (!addedPath.empty())
	{
		auto pathVar = addedPath + originalPathVar;
		Environment::setPath(pathVar);
	}

	return true;
}

/*****************************************************************************/
uint WorkspaceEnvironment::processorCount() const noexcept
{
	return m_processorCount;
}

/*****************************************************************************/
uint WorkspaceEnvironment::maxJobs() const noexcept
{
	return m_maxJobs;
}

void WorkspaceEnvironment::setMaxJobs(const uint inValue) noexcept
{
	m_maxJobs = std::clamp(inValue, 1U, processorCount());
}

/*****************************************************************************/
bool WorkspaceEnvironment::dumpAssembly() const noexcept
{
	return m_dumpAssembly;
}

void WorkspaceEnvironment::setDumpAssembly(const bool inValue) noexcept
{
	m_dumpAssembly = inValue;
}

/*****************************************************************************/
const std::string& WorkspaceEnvironment::workspace() const noexcept
{
	return m_workspace;
}

void WorkspaceEnvironment::setWorkspace(std::string&& inValue) noexcept
{
	m_workspace = std::move(inValue);
}

/*****************************************************************************/
const std::string& WorkspaceEnvironment::version() const noexcept
{
	return m_version;
}

void WorkspaceEnvironment::setVersion(std::string&& inValue) noexcept
{
	m_version = std::move(inValue);
}

/*****************************************************************************/
const StringList& WorkspaceEnvironment::searchPaths() const noexcept
{
	return m_searchPaths;
}

void WorkspaceEnvironment::addSearchPaths(StringList&& inList)
{
	List::forEach(inList, this, &WorkspaceEnvironment::addSearchPath);
}

void WorkspaceEnvironment::addSearchPath(std::string&& inValue)
{
	if (inValue.back() == '/')
		inValue.pop_back();

	List::addIfDoesNotExist(m_searchPaths, std::move(inValue));
}

/*****************************************************************************/
std::string WorkspaceEnvironment::makePathVariable(const std::string& inRootPath) const
{
	if (m_searchPaths.empty())
		return inRootPath;

	auto separator = Path::getSeparator();
	StringList outList;

	for (auto& p : m_searchPaths)
	{
		if (!Commands::pathExists(p))
			continue;

		auto path = Commands::getCanonicalPath(p); // for any relative paths

		if (!String::contains(path, inRootPath))
			outList.emplace_back(std::move(path));
	}

	if (outList.empty())
		return inRootPath;

	if (!inRootPath.empty())
	{
		outList.push_back(inRootPath);
	}

	std::string ret = String::join(std::move(outList), separator);
	Path::sanitize(ret);

	return ret;
}

}