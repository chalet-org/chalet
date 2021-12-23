/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WINDOWS_NULLSOFT_INSTALLER_RUNNER_HPP
#define CHALET_WINDOWS_NULLSOFT_INSTALLER_RUNNER_HPP

namespace chalet
{
struct CentralState;
struct WindowsNullsoftInstallerTarget;

struct WindowsNullsoftInstallerRunner
{
	explicit WindowsNullsoftInstallerRunner(const CentralState& inCentralState);

	bool compile(const WindowsNullsoftInstallerTarget& inTarget);

private:
	StringList getPluginPaths(const WindowsNullsoftInstallerTarget& inTarget);

	const CentralState& m_centralState;
};
}

#endif // CHALET_WINDOWS_NULLSOFT_INSTALLER_RUNNER_HPP
