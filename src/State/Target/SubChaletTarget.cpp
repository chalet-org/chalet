/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/SubChaletTarget.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Cache/WorkspaceInternalCacheFile.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Dependency/IExternalDependency.hpp"
#include "System/Files.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonValues.hpp"

namespace chalet
{
/*****************************************************************************/
SubChaletTarget::SubChaletTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::SubChalet)
{
}

/*****************************************************************************/
bool SubChaletTarget::initialize()
{
	if (!IBuildTarget::initialize())
		return false;

	if (!m_state.replaceVariablesInString(m_buildFile, this))
		return false;

	if (!m_state.replaceVariablesInString(m_location, this))
		return false;

	m_targetFolder = m_state.paths.getExternalBuildDir(this->name());
	Path::toUnix(m_targetFolder);

	if (m_targets.empty())
	{
		m_targets.emplace_back(Values::All);
	}

	return true;
}

/*****************************************************************************/
bool SubChaletTarget::validate()
{
	const auto& targetName = this->name();

	bool result = true;
	if (!Files::pathExists(m_location))
	{
		Diagnostic::error("location for Chalet target '{}' doesn't exist: {}", targetName, m_location);
		result = false;
	}

	if (!m_buildFile.empty() && !Files::pathExists(fmt::format("{}/{}", m_location, m_buildFile)))
	{
		Diagnostic::error("buildFile '{}' for Chalet target '{}' was not found in the location: {}", m_buildFile, targetName, m_location);
		result = false;
	}

	if (m_targets.empty())
	{
		Diagnostic::error("Chalet target '{}' did not contain any targets (expected at least 'all')", targetName);
		result = false;
	}

	return result;
}

/*****************************************************************************/
const std::string& SubChaletTarget::getHash() const
{
	if (m_hash.empty())
	{
		auto targets = String::join(m_targets);

		auto hashable = Hash::getHashableString(this->name(), m_location, m_targetFolder, m_buildFile, targets, m_recheck, m_rebuild, m_clean);

		m_hash = Hash::string(hashable);
	}

	return m_hash;
}

/*****************************************************************************/
bool SubChaletTarget::hashChanged() const noexcept
{
	if (m_hashChanged == -1)
	{
		bool externalChanged = false;
		if (String::startsWith(m_state.inputs.externalDirectory(), location()))
		{
			auto loc = location().substr(m_state.inputs.externalDirectory().size() + 1);
			for (auto& dep : m_state.externalDependencies)
			{
				const auto& name = dep->name();
				if (String::startsWith(name, loc))
				{
					externalChanged = dep->needsUpdate();
					break;
				}
			}
		}
		auto& sourceCache = m_state.cache.file().sources();
		bool cacheChanged = sourceCache.dataCacheValueChanged(Hash::string(fmt::format("chalet.{}", this->name())), m_hash);
		m_hashChanged = static_cast<i32>(cacheChanged || externalChanged);
	}
	return m_hashChanged == 1;
}

/*****************************************************************************/
const StringList& SubChaletTarget::targets() const noexcept
{
	return m_targets;
}
void SubChaletTarget::addTargets(StringList&& inList)
{
	List::forEach(inList, this, &SubChaletTarget::addTarget);
}
void SubChaletTarget::addTarget(std::string&& inValue)
{
	List::addIfDoesNotExist(m_targets, std::move(inValue));
}

/*****************************************************************************/
const std::string& SubChaletTarget::location() const noexcept
{
	return m_location;
}

const std::string& SubChaletTarget::targetFolder() const noexcept
{
	return m_targetFolder;
}

void SubChaletTarget::setLocation(std::string&& inValue) noexcept
{
	m_location = std::move(inValue);
}

/*****************************************************************************/
const std::string& SubChaletTarget::buildFile() const noexcept
{
	return m_buildFile;
}

void SubChaletTarget::setBuildFile(std::string&& inValue) noexcept
{
	m_buildFile = std::move(inValue);
}

/*****************************************************************************/
bool SubChaletTarget::recheck() const noexcept
{
	return m_recheck;
}

void SubChaletTarget::setRecheck(const bool inValue) noexcept
{
	m_recheck = inValue;
}

/*****************************************************************************/
bool SubChaletTarget::rebuild() const noexcept
{
	return m_rebuild;
}
void SubChaletTarget::setRebuild(const bool inValue) noexcept
{
	m_rebuild = inValue;
}

/*****************************************************************************/
bool SubChaletTarget::clean() const noexcept
{
	return m_clean;
}
void SubChaletTarget::setClean(const bool inValue) noexcept
{
	m_clean = inValue;
}

}
