/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WINDOWS_NULLSOFT_INSTALLER_RUNNER_HPP
#define CHALET_WINDOWS_NULLSOFT_INSTALLER_RUNNER_HPP

namespace chalet
{
class BuildState;
struct WindowsNullsoftInstallerTarget;

struct WindowsNullsoftInstallerRunner
{
	explicit WindowsNullsoftInstallerRunner(const BuildState& inState);

	bool compile(const WindowsNullsoftInstallerTarget& inTarget);

private:
	StringList getPluginPaths(const WindowsNullsoftInstallerTarget& inTarget);

	const BuildState& m_state;
};
}

#endif // CHALET_WINDOWS_NULLSOFT_INSTALLER_RUNNER_HPP
