/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/WorkspaceEnvironment.hpp"

#include "Libraries/FileSystem.hpp"

#include "Process/Environment.hpp"
#include "State/BuildState.hpp"
#include "State/TargetMetadata.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
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

		if (!inState.replaceVariablesInString(path, static_cast<const IBuildTarget*>(nullptr)))
			return false;

		Path::toUnix(path);

		addedPath += path;
		addedPath += Environment::getPathSeparator();
	}

	if (!addedPath.empty())
	{
		auto pathVar = addedPath + originalPathVar;
		Environment::setPath(pathVar);
	}

	if (!m_metadata->initialize(inState, nullptr, true))
		return false;

	return true;
}

/*****************************************************************************/
const TargetMetadata& WorkspaceEnvironment::metadata() const noexcept
{
	return *m_metadata;
}
void WorkspaceEnvironment::setMetadata(Ref<TargetMetadata>&& inValue)
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
	constexpr auto separator = Environment::getPathSeparator();
	StringList outList;
	StringList rootPaths = String::split(inRootPath, separator);

	for (auto& p : m_searchPaths)
	{
		// if (!Files::pathExists(p))
		// 	continue;

		auto path = Files::getCanonicalPath(p); // for any relative paths

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
	Path::toUnix(ret);

	return ret;
}

/*****************************************************************************/
std::string WorkspaceEnvironment::makePathVariable(const std::string& inRootPath, const StringList& inAdditionalPaths) const
{
	constexpr auto separator = Environment::getPathSeparator();
	StringList outList;
	StringList rootPaths = String::split(inRootPath, separator);

	for (auto& p : m_searchPaths)
	{
		// if (!Files::pathExists(p))
		// 	continue;

		auto path = Files::getCanonicalPath(p); // for any relative paths

		if (!List::contains(rootPaths, path))
			List::addIfDoesNotExist(outList, std::move(path));
	}

	for (auto& p : inAdditionalPaths)
	{
		// if (!Files::pathExists(p))
		// 	continue;

		auto path = Files::getCanonicalPath(p); // for any relative paths

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
	Path::toUnix(ret);

	return ret;
}
}