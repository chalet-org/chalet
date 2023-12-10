/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Package/SourcePackage.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "State/BuildState.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
SourcePackage::SourcePackage(const BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
bool SourcePackage::initialize()
{
	const auto globMessage = "Check that they exist and glob patterns can be resolved";
#if defined(CHALET_MACOS)
	if (!expandGlobPatternsInList(m_appleFrameworkPaths, GlobMatch::Folders))
	{
		Diagnostic::error("There was a problem resolving the macos framework paths for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}
#endif

	if (!expandGlobPatternsInList(m_libDirs, GlobMatch::Folders))
	{
		Diagnostic::error("There was a problem resolving the lib directories for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}

	if (!expandGlobPatternsInList(m_includeDirs, GlobMatch::Folders))
	{
		Diagnostic::error("There was a problem resolving the include directories for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}

	if (!expandGlobPatternsInList(m_copyFilesOnRun, GlobMatch::FilesAndFolders))
	{
		Diagnostic::error("There was a problem resolving the files to copy on run for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}

	if (!replaceVariablesInPathList(m_searchPaths))
		return false;

	if (!replaceVariablesInPathList(m_linkerOptions))
		return false;

	if (!replaceVariablesInPathList(m_links))
		return false;

	if (!replaceVariablesInPathList(m_staticLinks))
		return false;

#if defined(CHALET_MACOS)
	for (auto& path : m_appleFrameworkPaths)
		path = Files::getAbsolutePath(path);
#endif

	for (auto& path : m_libDirs)
		path = Files::getAbsolutePath(path);

	for (auto& path : m_includeDirs)
		path = Files::getAbsolutePath(path);

	for (auto& path : m_searchPaths)
		path = Files::getAbsolutePath(path);

	for (auto& path : m_copyFilesOnRun)
		path = Files::getAbsolutePath(path);

	auto sharedExt = m_state.environment->getSharedLibraryExtension();

	for (auto& path : m_links)
	{
		if (String::endsWith(sharedExt, path))
			path = Files::getAbsolutePath(path);
	}

	for (auto& path : m_staticLinks)
	{
		if (String::endsWith(sharedExt, path))
			path = Files::getAbsolutePath(path);
	}

	return true;
}

/*****************************************************************************/
const std::string& SourcePackage::name() const noexcept
{
	return m_name;
}
void SourcePackage::setName(const std::string& inValue) noexcept
{
	m_name = inValue;
}

/*****************************************************************************/
const std::string& SourcePackage::root() const noexcept
{
	return m_root;
}
void SourcePackage::setRoot(const std::string& inValue) noexcept
{
	m_root = inValue;
}

/*****************************************************************************/
const StringList& SourcePackage::searchPaths() const noexcept
{
	return m_searchPaths;
}
void SourcePackage::addSearchPaths(StringList&& inList)
{
	List::forEach(inList, this, &SourcePackage::addSearchPath);
}
void SourcePackage::addSearchPath(std::string&& inValue)
{
	List::addIfDoesNotExist(m_searchPaths, inValue);
}

/*****************************************************************************/
const StringList& SourcePackage::copyFilesOnRun() const noexcept
{
	return m_copyFilesOnRun;
}
void SourcePackage::addCopyFilesOnRun(StringList&& inList)
{
	List::forEach(inList, this, &SourcePackage::addCopyFileOnRun);
}
void SourcePackage::addCopyFileOnRun(std::string&& inValue)
{
	List::addIfDoesNotExist(m_copyFilesOnRun, inValue);
}

/*****************************************************************************/
const StringList& SourcePackage::libDirs() const noexcept
{
	return m_libDirs;
}
void SourcePackage::addLibDirs(StringList&& inList)
{
	List::forEach(inList, this, &SourcePackage::addLibDir);
}
void SourcePackage::addLibDir(std::string&& inValue)
{
	List::addIfDoesNotExist(m_libDirs, inValue);
}

/*****************************************************************************/
const StringList& SourcePackage::includeDirs() const noexcept
{
	return m_includeDirs;
}
void SourcePackage::addIncludeDirs(StringList&& inList)
{
	List::forEach(inList, this, &SourcePackage::addIncludeDir);
}
void SourcePackage::addIncludeDir(std::string&& inValue)
{
	List::addIfDoesNotExist(m_includeDirs, inValue);
}

/*****************************************************************************/
const StringList& SourcePackage::links() const noexcept
{
	return m_links;
}
void SourcePackage::addLinks(StringList&& inList)
{
	List::forEach(inList, this, &SourcePackage::addLink);
}
void SourcePackage::addLink(std::string&& inValue)
{
	List::addIfDoesNotExist(m_links, inValue);
}

/*****************************************************************************/
const StringList& SourcePackage::staticLinks() const noexcept
{
	return m_staticLinks;
}
void SourcePackage::addStaticLinks(StringList&& inList)
{
	List::forEach(inList, this, &SourcePackage::addStaticLink);
}
void SourcePackage::addStaticLink(std::string&& inValue)
{
	List::addIfDoesNotExist(m_staticLinks, inValue);
}

/*****************************************************************************/
const StringList& SourcePackage::linkerOptions() const noexcept
{
	return m_linkerOptions;
}
void SourcePackage::addLinkerOptions(StringList&& inList)
{
	List::forEach(inList, this, &SourcePackage::addLinkerOption);
}
void SourcePackage::addLinkerOption(std::string&& inValue)
{
	List::addIfDoesNotExist(m_linkerOptions, inValue);
}

/*****************************************************************************/
const StringList& SourcePackage::appleFrameworkPaths() const noexcept
{
	return m_appleFrameworkPaths;
}
void SourcePackage::addAppleFrameworkPaths(StringList&& inList)
{
	List::forEach(inList, this, &SourcePackage::addAppleFrameworkPath);
}
void SourcePackage::addAppleFrameworkPath(std::string&& inValue)
{
	List::addIfDoesNotExist(m_appleFrameworkPaths, inValue);
}

/*****************************************************************************/
const StringList& SourcePackage::appleFrameworks() const noexcept
{
	return m_appleFrameworks;
}
void SourcePackage::addAppleFrameworks(StringList&& inList)
{
	List::forEach(inList, this, &SourcePackage::addAppleFramework);
}
void SourcePackage::addAppleFramework(std::string&& inValue)
{
	List::addIfDoesNotExist(m_appleFrameworks, inValue);
}

/*****************************************************************************/
bool SourcePackage::replaceVariablesInPathList(StringList& outList) const
{
	for (auto& dir : outList)
	{
		if (!m_state.replaceVariablesInString(dir, this))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool SourcePackage::expandGlobPatternsInList(StringList& outList, GlobMatch inSettings) const
{
	if (outList.empty())
		return true;

	StringList list = outList;
	if (!replaceVariablesInPathList(list))
		return false;

	outList.clear();
	for (const auto& val : list)
	{
		if (!Files::addPathToListWithGlob(val, outList, inSettings))
			return false;
	}

	return true;
}

}
