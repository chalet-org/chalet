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
	explicit PackageManager(const BuildState& inState);

	bool initialize();

	void add(const std::string& inName, Ref<SourcePackage>&& inValue);

private:
	const BuildState& m_state;

	Dictionary<Ref<SourcePackage>> m_packages;
};
}
