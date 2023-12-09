/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Utility/GlobMatch.hpp"

namespace chalet
{
class BuildState;

struct SourcePackage
{
	explicit SourcePackage(const BuildState& inState);

	bool initialize();

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

	const std::string& root() const noexcept;
	void setRoot(const std::string& inValue) noexcept;

	const StringList& binaries() const noexcept;
	void addBinaries(StringList&& inList);
	void addBinary(std::string&& inValue);

	const StringList& searchDirs() const noexcept;
	void addSearchDirs(StringList&& inList);
	void addSearchDir(std::string&& inValue);

	const StringList& libDirs() const noexcept;
	void addLibDirs(StringList&& inList);
	void addLibDir(std::string&& inValue);

	const StringList& includeDirs() const noexcept;
	void addIncludeDirs(StringList&& inList);
	void addIncludeDir(std::string&& inValue);

	const StringList& links() const noexcept;
	void addLinks(StringList&& inList);
	void addLink(std::string&& inValue);

	const StringList& staticLinks() const noexcept;
	void addStaticLinks(StringList&& inList);
	void addStaticLink(std::string&& inValue);

	const StringList& linkerOptions() const noexcept;
	void addLinkerOptions(StringList&& inList);
	void addLinkerOption(std::string&& inValue);

	const StringList& appleFrameworkPaths() const noexcept;
	void addAppleFrameworkPaths(StringList&& inList);
	void addAppleFrameworkPath(std::string&& inValue);

	const StringList& appleFrameworks() const noexcept;
	void addAppleFrameworks(StringList&& inList);
	void addAppleFramework(std::string&& inValue);

private:
	bool replaceVariablesInPathList(StringList& outList) const;
	bool expandGlobPatternsInList(StringList& outList, GlobMatch inSettings) const;

	const BuildState& m_state;

	std::string m_name;
	std::string m_root;

	StringList m_binaries;
	StringList m_searchDirs;
	StringList m_links;
	StringList m_staticLinks;
	StringList m_libDirs;
	StringList m_includeDirs;
	StringList m_linkerOptions;
	StringList m_appleFrameworkPaths;
	StringList m_appleFrameworks;
};
}
