/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/WorkspaceEnvironment.hpp"

#include "Libraries/FileSystem.hpp"

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
bool WorkspaceEnvironment::initialize(BuildPaths& inPaths)
{
	auto originalPathVar = Environment::getPath();
	std::string addedPath;
	for (auto& path : m_searchPaths)
	{
		if (String::contains(path + Environment::getPathSeparator(), originalPathVar))
			continue;

		inPaths.replaceVariablesInPath(path);
		addedPath += path;
		addedPath += Environment::getPathSeparator();
	}

	if (!addedPath.empty())
	{
		auto pathVar = addedPath + originalPathVar;
		Environment::setPath(pathVar);
	}

	return true;
}

/*****************************************************************************/
const std::string& WorkspaceEnvironment::workspaceName() const noexcept
{
	return m_workspaceName;
}

void WorkspaceEnvironment::setWorkspaceName(std::string&& inValue) noexcept
{
	m_workspaceName = std::move(inValue);
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
	auto separator = Environment::getPathSeparator();
	StringList outList;
	StringList rootPaths = String::split(inRootPath, separator);

	for (auto& p : m_searchPaths)
	{
		if (!Commands::pathExists(p))
			continue;

		auto path = Commands::getCanonicalPath(p); // for any relative paths

		if (!String::contains(path, inRootPath))
			List::addIfDoesNotExist(outList, std::move(path));
	}

	if (outList.empty())
		return inRootPath;

	for (auto& p : rootPaths)
	{
		outList.push_back(std::move(p));
	}

	std::string ret = String::join(std::move(outList), separator);
	Path::sanitize(ret);

	return ret;
}

/*****************************************************************************/
std::string WorkspaceEnvironment::makePathVariable(const std::string& inRootPath, const StringList& inAdditionalPaths) const
{
	auto separator = Environment::getPathSeparator();
	StringList outList;
	StringList rootPaths = String::split(inRootPath, separator);

	for (auto& p : m_searchPaths)
	{
		if (!Commands::pathExists(p))
			continue;

		auto path = Commands::getCanonicalPath(p); // for any relative paths

		if (!List::contains(rootPaths, path))
			List::addIfDoesNotExist(outList, std::move(path));
	}

	for (auto& p : inAdditionalPaths)
	{
		if (!Commands::pathExists(p))
			continue;

		auto path = Commands::getCanonicalPath(p); // for any relative paths

		if (!List::contains(rootPaths, path))
			List::addIfDoesNotExist(outList, std::move(path));
	}

	if (outList.empty())
		return inRootPath;

	for (auto& p : rootPaths)
	{
		outList.push_back(std::move(p));
	}

	std::string ret = String::join(std::move(outList), separator);
	Path::sanitize(ret);

	return ret;
}
}