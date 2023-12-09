/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;
struct SourcePackage;

struct PackageManager
{
	explicit PackageManager(BuildState& inState);

	bool initialize();

	bool add(const std::string& inName, Ref<SourcePackage>&& inValue);

	const StringList& packagePaths() const noexcept;
	void addPackagePaths(StringList&& inList);
	void addPackagePath(std::string&& inValue);

private:
	bool resolvePackagesFromSubChaletTargets();
	bool initializePackages();
	bool readImportedPackages();

	BuildState& m_state;

	StringList m_packagePaths;

	Dictionary<Ref<SourcePackage>> m_packages;
};
}
