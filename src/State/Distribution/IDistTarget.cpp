/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/IDistTarget.hpp"

#include "State/Distribution/BundleArchiveTarget.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/MacosDiskImageTarget.hpp"
#include "State/Distribution/ScriptDistTarget.hpp"
#include "State/Distribution/WindowsNullsoftInstallerTarget.hpp"

namespace chalet
{
/*****************************************************************************/
IDistTarget::IDistTarget(const DistTargetType inType) :
	m_type(inType)
{
}

/*****************************************************************************/
[[nodiscard]] DistTarget IDistTarget::make(const DistTargetType inType)
{
	switch (inType)
	{
		case DistTargetType::DistributionBundle:
			return std::make_unique<BundleTarget>();
		case DistTargetType::Script:
			return std::make_unique<ScriptDistTarget>();
		case DistTargetType::BundleArchive:
			return std::make_unique<BundleArchiveTarget>();
#if defined(CHALET_MACOS)
		case DistTargetType::MacosDiskImage:
			return std::make_unique<MacosDiskImageTarget>();
#endif
#if defined(CHALET_WIN32)
		case DistTargetType::WindowsNullsoftInstaller:
			return std::make_unique<WindowsNullsoftInstallerTarget>();
#endif
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented DistTargetType requested for makeBundle: ", static_cast<int>(inType));
	return nullptr;
}

/*****************************************************************************/
DistTargetType IDistTarget::type() const noexcept
{
	return m_type;
}
bool IDistTarget::isScript() const noexcept
{
	return m_type == DistTargetType::Script;
}
bool IDistTarget::isDistributionBundle() const noexcept
{
	return m_type == DistTargetType::DistributionBundle;
}
bool IDistTarget::isArchive() const noexcept
{
	return m_type == DistTargetType::BundleArchive;
}
bool IDistTarget::isMacosDiskImage() const noexcept
{
	return m_type == DistTargetType::MacosDiskImage;
}
bool IDistTarget::isWindowsNullsoftInstaller() const noexcept
{
	return m_type == DistTargetType::WindowsNullsoftInstaller;
}

/*****************************************************************************/
const std::string& IDistTarget::name() const noexcept
{
	return m_name;
}
void IDistTarget::setName(const std::string& inValue) noexcept
{
	m_name = inValue;
}

/*****************************************************************************/
const std::string& IDistTarget::description() const noexcept
{
	return m_description;
}
void IDistTarget::setDescription(std::string&& inValue) noexcept
{
	m_description = std::move(inValue);
}

}
