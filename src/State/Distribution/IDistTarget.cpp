/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/IDistTarget.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "State/BuildState.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

#include "State/Distribution/BundleArchiveTarget.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/MacosDiskImageTarget.hpp"
#include "State/Distribution/ProcessDistTarget.hpp"
#include "State/Distribution/ScriptDistTarget.hpp"
#include "State/Distribution/ValidationDistTarget.hpp"

namespace chalet
{
/*****************************************************************************/
IDistTarget::IDistTarget(const BuildState& inState, const DistTargetType inType) :
	m_state(inState),
	m_type(inType)
{
}

/*****************************************************************************/
[[nodiscard]] DistTarget IDistTarget::make(const DistTargetType inType, const BuildState& inState)
{
	switch (inType)
	{
		case DistTargetType::DistributionBundle:
			return std::make_unique<BundleTarget>(inState);
		case DistTargetType::BundleArchive:
			return std::make_unique<BundleArchiveTarget>(inState);
#if defined(CHALET_MACOS)
		case DistTargetType::MacosDiskImage:
			return std::make_unique<MacosDiskImageTarget>(inState);
#endif
		case DistTargetType::Script:
			return std::make_unique<ScriptDistTarget>(inState);
		case DistTargetType::Process:
			return std::make_unique<ProcessDistTarget>(inState);
		case DistTargetType::Validation:
			return std::make_unique<ValidationDistTarget>(inState);
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented DistTargetType requested: {}", static_cast<i32>(inType));
	return nullptr;
}

/*****************************************************************************/
DistTargetType IDistTarget::type() const noexcept
{
	return m_type;
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
bool IDistTarget::isScript() const noexcept
{
	return m_type == DistTargetType::Script;
}
bool IDistTarget::isProcess() const noexcept
{
	return m_type == DistTargetType::Process;
}
bool IDistTarget::isValidation() const noexcept
{
	return m_type == DistTargetType::Validation;
}

/*****************************************************************************/
bool IDistTarget::replaceVariablesInPathList(StringList& outList) const
{
	for (auto& dir : outList)
	{
		if (!m_state.replaceVariablesInString(dir, this))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool IDistTarget::processEachPathList(StringList inList, std::function<bool(std::string&& inValue)> onProcess) const
{
	if (!replaceVariablesInPathList(inList))
		return false;

	for (auto&& val : inList)
	{
		if (!onProcess(std::move(val)))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool IDistTarget::processIncludeExceptions(StringList& outList) const
{
	if (m_state.environment->isEmscripten())
	{
		StringList additions;
		auto ext = m_state.environment->getExecutableExtension();
		for (auto& file : outList)
		{
			if (String::endsWith(ext, file))
			{
				auto noExtension = String::getPathFolderBaseName(file);
				additions.emplace_back(fmt::format("{}.wasm", noExtension));
				additions.emplace_back(fmt::format("{}.js", noExtension));
			}
		}

		for (auto& file : additions)
			List::addIfDoesNotExist(outList, std::move(file));
	}

	return true;
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
