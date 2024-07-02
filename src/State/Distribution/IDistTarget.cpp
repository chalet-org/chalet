/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/IDistTarget.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "System/Files.hpp"
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
bool IDistTarget::resolveDependentTargets(std::string& outDepends, std::string& outPath, const char* inKey) const
{
	if (!outDepends.empty())
	{
		if (String::equals(this->name(), outDepends))
		{
			Diagnostic::error("The distribution target '{}' depends on itself. Remove it from '{}'", this->name(), inKey);
			return false;
		}

		bool found = false;
		for (auto& target : m_state.distribution)
		{
			if (String::equals(target->name(), this->name()))
				break;

			if (String::equals(target->name(), outDepends))
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			Diagnostic::error("The distribution target '{}' depends on the '{}' target which either doesn't exist or sequenced later.", this->name(), outDepends);
			return false;
		}
	}

	if (!Files::pathExists(outPath))
	{
		auto resolved = Files::which(outPath);
		if (resolved.empty() && outDepends.empty())
		{
			bool inBuild = String::startsWith(m_state.paths.buildOutputDir(), outPath);
			if (!inBuild)
			{
				Diagnostic::error("The path for the distribution target '{}' doesn't exist: {}", this->name(), outPath);
				return false;
			}
			else
			{
#if defined(CHALET_WIN32)
				auto exe = Files::getPlatformExecutableExtension();
				if (!exe.empty() && !String::endsWith(exe, outPath))
				{
					outPath += exe;
				}
#endif
			}
		}

		if (!resolved.empty())
			outPath = std::move(resolved);
	}

	return true;
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
bool IDistTarget::expandGlobPatternsInList(StringList& outList, GlobMatch inSettings) const
{
	StringList list = outList;
	if (!replaceVariablesInPathList(list))
		return false;

	outList.clear();
	for (const auto& val : list)
	{
		if (!Files::addPathToListWithGlob(val, outList, inSettings))
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
