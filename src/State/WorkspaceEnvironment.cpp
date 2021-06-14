/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/WorkspaceEnvironment.hpp"

#include <thread>

#include "Libraries/FileSystem.hpp"
#include "Libraries/Format.hpp"
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
void WorkspaceEnvironment::initialize(BuildPaths& inPaths)
{
	std::string addedPath;
	for (auto& path : m_path)
	{
		inPaths.replaceVariablesInPath(path);
		addedPath += path;
		addedPath += Path::getSeparator();
	}

	if (!addedPath.empty())
	{
		auto pathVar = addedPath + Environment::getPath();
		Environment::setPath(pathVar);
	}
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
	m_maxJobs = std::min(inValue, processorCount());
}

/*****************************************************************************/
bool WorkspaceEnvironment::showCommands() const noexcept
{
	return m_showCommands;
}

void WorkspaceEnvironment::setShowCommands(const bool inValue) noexcept
{
	m_showCommands = inValue;
}

bool WorkspaceEnvironment::cleanOutput() const noexcept
{
	return !m_showCommands;
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
const StringList& WorkspaceEnvironment::path() const noexcept
{
	return m_path;
}

void WorkspaceEnvironment::addPaths(StringList&& inList)
{
	List::forEach(inList, this, &WorkspaceEnvironment::addPath);
}

void WorkspaceEnvironment::addPath(std::string&& inValue)
{
	if (inValue.back() == '/')
		inValue.pop_back();

	List::addIfDoesNotExist(m_path, std::move(inValue));
}

/*****************************************************************************/
std::string WorkspaceEnvironment::makePathVariable(const std::string& inRootPath)
{
	auto separator = Path::getSeparator();

	StringList outList;

	for (auto& p : m_path)
	{
		if (!Commands::pathExists(p))
			continue;

		auto path = Commands::getCanonicalPath(p); // for any relative paths

		if (!String::contains(path, inRootPath))
			outList.push_back(std::move(path));
	}

	outList.push_back(inRootPath);

	std::string ret = String::join(outList, separator);
	Path::sanitize(ret);

	return ret;
}

}