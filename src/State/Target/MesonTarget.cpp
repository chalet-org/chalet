/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/MesonTarget.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Cache/WorkspaceInternalCacheFile.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Dependency/IExternalDependency.hpp"
#include "System/Files.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
MesonTarget::MesonTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::Meson)
{
}

/*****************************************************************************/
bool MesonTarget::initialize()
{
	if (!IBuildTarget::initialize())
		return false;

	if (!m_state.replaceVariablesInString(m_buildFile, this))
		return false;

	if (!m_state.replaceVariablesInString(m_location, this))
		return false;

	if (!replaceVariablesInPathList(m_defines))
		return false;

	if (!m_state.replaceVariablesInString(m_runExecutable, this))
		return false;

	m_targetFolder = m_state.paths.getExternalBuildDir(this->name());
	Path::toUnix(m_targetFolder);

	// Note: this technically gets handled by ninja, but for correctness
	for (auto& target : m_targets)
	{
#if defined(CHALET_WIN32)
		String::replaceAll(target, '/', '\\');
#else
		String::replaceAll(target, '\\', '/');
#endif
	}

	return true;
}

/*****************************************************************************/
bool MesonTarget::validate()
{
	const auto& targetName = this->name();

	bool result = true;
	if (!Files::pathExists(m_location))
	{
		Diagnostic::error("location for Meson target '{}' doesn't exist: {}", targetName, m_location);
		result = false;
	}

	if (!m_buildFile.empty() && !Files::pathExists(fmt::format("{}/{}", m_location, m_buildFile)))
	{
		Diagnostic::error("buildFile '{}' for Meson target '{}' was not found in the location: {}", m_buildFile, targetName, m_location);
		result = false;
	}

	for (auto& define : m_defines)
	{
		if (!String::contains('=', define))
		{
			Diagnostic::error("define '{}' for Meson target '{}' must use the format 'key=value'", define, targetName);
			result = false;
		}
	}

	if (!m_state.toolchain.mesonAvailable())
	{
		Diagnostic::error("Meson was required for the project '{}' but was not found.", this->name());
		result = false;
	}

	return result;
}

/*****************************************************************************/
const std::string& MesonTarget::getHash() const
{
	if (m_hash.empty())
	{
		auto defines = String::join(m_defines);
		auto targets = String::join(m_targets);

		bool compilerCache = m_state.info.compilerCache(); // we also need to re-configure meson if the compilerCache flag was changed

		auto hashable = Hash::getHashableString(this->name(), m_location, m_runExecutable, m_buildFile, m_toolset, m_install, defines, targets, compilerCache);

		m_hash = Hash::string(hashable);
	}

	return m_hash;
}

/*****************************************************************************/
bool MesonTarget::hashChanged() const noexcept
{
	if (m_hashChanged == -1)
	{
		auto dependency = m_state.getExternalDependencyFromLocation(m_location);

		auto& sourceCache = m_state.cache.file().sources();
		auto configHash = m_state.configuration.getHash();
		auto dependencyHash = dependency != nullptr ? dependency->getHash() : std::string();
		bool cacheChanged = sourceCache.dataCacheValueChanged(Hash::string(fmt::format("meson.{}.{}", dependencyHash, configHash)), m_hash);
		m_hashChanged = static_cast<i32>(cacheChanged);
	}
	return m_hashChanged == 1;
}

/*****************************************************************************/
const StringList& MesonTarget::defines() const noexcept
{
	return m_defines;
}

void MesonTarget::addDefines(StringList&& inList)
{
	List::forEach(inList, this, &MesonTarget::addDefine);
}

void MesonTarget::addDefine(std::string&& inValue)
{
	List::addIfDoesNotExist(m_defines, std::move(inValue));
}

/*****************************************************************************/
const StringList& MesonTarget::targets() const noexcept
{
	return m_targets;
}
void MesonTarget::addTargets(StringList&& inList)
{
	List::forEach(inList, this, &MesonTarget::addTarget);
}
void MesonTarget::addTarget(std::string&& inValue)
{
	List::addIfDoesNotExist(m_targets, std::move(inValue));
}

/*****************************************************************************/
const std::string& MesonTarget::buildFile() const noexcept
{
	return m_buildFile;
}

void MesonTarget::setBuildFile(std::string&& inValue) noexcept
{
	m_buildFile = std::move(inValue);
}

/*****************************************************************************/
const std::string& MesonTarget::location() const noexcept
{
	return m_location;
}

const std::string& MesonTarget::targetFolder() const noexcept
{
	return m_targetFolder;
}

void MesonTarget::setLocation(std::string&& inValue) noexcept
{
	m_location = std::move(inValue);
}

/*****************************************************************************/
const std::string& MesonTarget::runExecutable() const noexcept
{
	return m_runExecutable;
}

void MesonTarget::setRunExecutable(std::string&& inValue) noexcept
{
	m_runExecutable = std::move(inValue);
}

/*****************************************************************************/
bool MesonTarget::recheck() const noexcept
{
	return m_recheck;
}

void MesonTarget::setRecheck(const bool inValue) noexcept
{
	m_recheck = inValue;
}

/*****************************************************************************/
bool MesonTarget::rebuild() const noexcept
{
	return m_rebuild;
}
void MesonTarget::setRebuild(const bool inValue) noexcept
{
	m_rebuild = inValue;
}

/*****************************************************************************/
bool MesonTarget::clean() const noexcept
{
	return m_clean;
}
void MesonTarget::setClean(const bool inValue) noexcept
{
	m_clean = inValue;
}

/*****************************************************************************/
bool MesonTarget::install() const noexcept
{
	return m_install;
}
void MesonTarget::setInstall(const bool inValue) noexcept
{
	m_install = inValue;
}

}
