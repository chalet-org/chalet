/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/WorkspaceEnvironment.hpp"

#include "Libraries/FileSystem.hpp"

#include "State/BuildState.hpp"
#include "State/TargetMetadata.hpp"
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
	m_metadata(std::make_shared<TargetMetadata>())
{
}

/*****************************************************************************/
bool WorkspaceEnvironment::initialize(const BuildState& inState)
{
	auto originalPathVar = Environment::getPath();
	std::string addedPath;
	for (auto& path : m_searchPaths)
	{
		if (String::contains(path + Environment::getPathSeparator(), originalPathVar))
			continue;

		inState.replaceVariablesInString(path, static_cast<const IBuildTarget*>(nullptr));
		addedPath += path;
		addedPath += Environment::getPathSeparator();
	}

	if (!addedPath.empty())
	{
		auto pathVar = addedPath + originalPathVar;
		Environment::setPath(pathVar);
	}

	if (!m_metadata->initialize())
		return false;

	return true;
}

/*****************************************************************************/
const TargetMetadata& WorkspaceEnvironment::metadata() const noexcept
{
	return *m_metadata;
}
void WorkspaceEnvironment::setMetadata(Shared<TargetMetadata>&& inValue)
{
	m_metadata = std::move(inValue);
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
		List::addIfDoesNotExist(outList, std::move(p));
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
		List::addIfDoesNotExist(outList, std::move(p));
	}

	std::string ret = String::join(std::move(outList), separator);
	Path::sanitize(ret);

	return ret;
}
}