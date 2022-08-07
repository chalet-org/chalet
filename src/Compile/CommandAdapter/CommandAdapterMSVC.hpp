/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMAND_ADAPTER_MSVC_HPP
#define CHALET_COMMAND_ADAPTER_MSVC_HPP

#include "State/MSVCWarningLevel.hpp"

namespace chalet
{
class BuildState;
struct SourceTarget;

class CommandAdapterMSVC
{
public:
	CommandAdapterMSVC(const BuildState& inState, const SourceTarget& inProject);

	MSVCWarningLevel getWarningLevel() const;

	std::string getLanguageStandardCpp() const;
	std::string getLanguageStandardC() const;
	std::string getSubSystem() const;
	std::string getEntryPoint() const;

private:
	const BuildState& m_state;
	const SourceTarget& m_project;

	uint m_versionMajorMinor = 0;
	uint m_versionPatch = 0;
};
}

#endif // CHALET_COMMAND_ADAPTER_MSVC_HPP
