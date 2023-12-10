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
	CHALET_DISALLOW_COPY_MOVE(PackageManager);
	~PackageManager();

	bool initialize();

	bool add(const std::string& inName, Ref<SourcePackage>&& inValue);

	const StringList& packagePaths() const noexcept;
	void addPackagePaths(StringList&& inList);
	void addPackagePath(std::string&& inValue);

	void addRequiredPackage(const std::string& inName);

	void addPackageDependencies(const std::string& inName, StringList&& inList);
	void addPackageDependency(const std::string& inName, std::string&& inValue);

private:
	bool resolvePackagesFromSubChaletTargets();
	bool validatePackageDependencies();
	bool initializePackages();
	bool readImportedPackages();
	void resolveDependencies(const std::string& package, StringList& outPackages);

	BuildState& m_state;

	struct Impl;
	Unique<Impl> m_impl;
};
}
