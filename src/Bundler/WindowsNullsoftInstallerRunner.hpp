/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WINDOWS_NULLSOFT_INSTALLER_RUNNER_HPP
#define CHALET_WINDOWS_NULLSOFT_INSTALLER_RUNNER_HPP

namespace chalet
{
struct StatePrototype;
struct WindowsNullsoftInstallerTarget;

struct WindowsNullsoftInstallerRunner
{
	explicit WindowsNullsoftInstallerRunner(const StatePrototype& inPrototype);

	bool compile(const WindowsNullsoftInstallerTarget& inTarget);

private:
	const StatePrototype& m_prototype;
};
}

#endif // CHALET_WINDOWS_NULLSOFT_INSTALLER_RUNNER_HPP
