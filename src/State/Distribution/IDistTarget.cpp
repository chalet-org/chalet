/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/IDistTarget.hpp"

#include "State/CentralState.hpp"
#include "State/Distribution/BundleArchiveTarget.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/MacosDiskImageTarget.hpp"
#include "State/Distribution/ProcessDistTarget.hpp"
#include "State/Distribution/ScriptDistTarget.hpp"
#include "State/Distribution/WindowsNullsoftInstallerTarget.hpp"

namespace chalet
{
/*****************************************************************************/
IDistTarget::IDistTarget(const CentralState& inCentralState, const DistTargetType inType) :
	m_centralState(inCentralState),
	m_type(inType)
{
}

/*****************************************************************************/
[[nodiscard]] DistTarget IDistTarget::make(const DistTargetType inType, const CentralState& inCentralState)
{
	switch (inType)
	{
		case DistTargetType::DistributionBundle:
			return std::make_unique<BundleTarget>(inCentralState);
		case DistTargetType::Script:
			return std::make_unique<ScriptDistTarget>(inCentralState);
		case DistTargetType::Process:
			return std::make_unique<ProcessDistTarget>(inCentralState);
		case DistTargetType::BundleArchive:
			return std::make_unique<BundleArchiveTarget>(inCentralState);
#if defined(CHALET_MACOS)
		case DistTargetType::MacosDiskImage:
			return std::make_unique<MacosDiskImageTarget>(inCentralState);
#endif
#if defined(CHALET_WIN32) || defined(CHALET_LINUX)
		case DistTargetType::WindowsNullsoftInstaller:
			return std::make_unique<WindowsNullsoftInstallerTarget>(inCentralState);
#endif
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented DistTargetType requested: ", static_cast<int>(inType));
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
bool IDistTarget::isProcess() const noexcept
{
	return m_type == DistTargetType::Process;
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
void IDistTarget::replaceVariablesInPathList(StringList& outList)
{
	for (auto& dir : outList)
	{
		m_centralState.replaceVariablesInPath(dir, this);
	}
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
const std::string& IDistTarget::outputDescription() const noexcept
{
	return m_outputDescription;
}
void IDistTarget::setOutputDescription(std::string&& inValue) noexcept
{
	m_outputDescription = std::move(inValue);
}

/*****************************************************************************/
bool IDistTarget::includeInDistribution() const noexcept
{
	return m_includeInDistribution;
}
void IDistTarget::setIncludeInDistribution(const bool inValue)
{
	m_includeInDistribution &= inValue;
}
}
