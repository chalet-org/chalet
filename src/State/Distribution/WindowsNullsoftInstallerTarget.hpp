/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WINDOWS_NULLSOFT_INSTALLER_TARGET_HPP
#define CHALET_WINDOWS_NULLSOFT_INSTALLER_TARGET_HPP

#include "State/Distribution/IDistTarget.hpp"

namespace chalet
{
struct WindowsNullsoftInstallerTarget final : public IDistTarget
{
	explicit WindowsNullsoftInstallerTarget();

	virtual bool validate() final;

	const std::string& nsisScript() const noexcept;
	void setNsisScript(std::string&& inValue);

private:
	std::string m_nsisScript;
};
}

#endif // CHALET_WINDOWS_NULLSOFT_INSTALLER_TARGET_HPP
