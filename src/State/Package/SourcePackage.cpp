/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Package/SourcePackage.hpp"

#include "State/BuildState.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"

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
	// Note: this only works correctly in-project
	//
	if (!replaceVariablesInPathList(m_binaries))
		return false;

	const auto globMessage = "Check that they exist and glob patterns can be resolved";
	if (!expandGlobPatternsInList(m_appleFrameworkPaths, GlobMatch::Folders))
	{
		Diagnostic::error("There was a problem resolving the macos framework paths for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}

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

	if (!replaceVariablesInPathList(m_searchDirs))
		return false;

	if (!replaceVariablesInPathList(m_linkerOptions))
		return false;

	if (!replaceVariablesInPathList(m_links))
		return false;

	if (!replaceVariablesInPathList(m_staticLinks))
		return false;

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
const StringList& SourcePackage::binaries() const noexcept
{
	return m_binaries;
}
void SourcePackage::addBinaries(StringList&& inList)
{
	List::forEach(inList, this, &SourcePackage::addBinary);
}
void SourcePackage::addBinary(std::string&& inValue)
{
	List::addIfDoesNotExist(m_binaries, inValue);
}

/*****************************************************************************/
const StringList& SourcePackage::searchDirs() const noexcept
{
	return m_searchDirs;
}
void SourcePackage::addSearchDirs(StringList&& inList)
{
	List::forEach(inList, this, &SourcePackage::addSearchDir);
}
void SourcePackage::addSearchDir(std::string&& inValue)
{
	List::addIfDoesNotExist(m_searchDirs, inValue);
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
