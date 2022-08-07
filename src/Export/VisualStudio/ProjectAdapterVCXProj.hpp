/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROJECT_ADAPTER_VCXPROJ_HPP
#define CHALET_PROJECT_ADAPTER_VCXPROJ_HPP

#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"

namespace chalet
{
class BuildState;
struct SourceTarget;

class ProjectAdapterVCXProj
{
public:
	ProjectAdapterVCXProj(const BuildState& inState, const SourceTarget& inProject);

	bool isAtLeastVS2017() const;

	std::string getWarningLevel() const;
	std::string getSubSystem() const;
	std::string getEntryPoint() const;

	StringList getSourceExtensions() const;
	StringList getHeaderExtensions() const;
	StringList getResourceExtensions() const;

private:
	const BuildState& m_state;
	const SourceTarget& m_project;

	CommandAdapterMSVC m_msvcAdapter;

	uint m_versionMajorMinor = 0;
	uint m_versionPatch = 0;
};
}

#endif // CHALET_PROJECT_ADAPTER_VCXPROJ_HPP
