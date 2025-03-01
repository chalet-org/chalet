/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/CMakeTarget.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Cache/WorkspaceInternalCacheFile.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
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
CMakeTarget::CMakeTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::CMake)
{
}

/*****************************************************************************/
bool CMakeTarget::initialize()
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

	// This prevents some irritating duplications the user would have to write otherwise
	{
		const bool isMsvc = m_state.environment->isMsvc();
		StringList kFlagsInit{
			"CMAKE_C_FLAGS_INIT=",
			"CMAKE_CXX_FLAGS_INIT="
		};
		for (auto& define : m_defines)
		{
			if (String::startsWith(kFlagsInit, define))
			{
				if (isMsvc)
				{
					String::replaceAll(define, "-D", "/D");
					String::replaceAll(define, "-I", "/I");
				}
				else
				{
					String::replaceAll(define, "/D", "-D");
					String::replaceAll(define, "/I", "-I");
				}
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool CMakeTarget::validate()
{
	const auto& targetName = this->name();

	bool result = true;
	if (!Files::pathExists(m_location))
	{
		Diagnostic::error("location for CMake target '{}' doesn't exist: {}", targetName, m_location);
		result = false;
	}

	if (!m_buildFile.empty() && !Files::pathExists(fmt::format("{}/{}", m_location, m_buildFile)))
	{
		Diagnostic::error("buildFile '{}' for CMake target '{}' was not found in the location: {}", m_buildFile, targetName, m_location);
		result = false;
	}

	if (!m_state.toolchain.cmakeAvailable())
	{
		Diagnostic::error("CMake was required for the project '{}' but was not found.", this->name());
		result = false;
	}

	return result;
}

/*****************************************************************************/
const std::string& CMakeTarget::getHash() const
{
	if (m_hash.empty())
	{
		auto defines = String::join(m_defines);
		auto targets = String::join(m_targets);

		auto hashable = Hash::getHashableString(this->name(), m_location, m_runExecutable, m_buildFile, m_toolset, defines, targets, m_install);

		m_hash = Hash::string(hashable);
	}

	return m_hash;
}

/*****************************************************************************/
bool CMakeTarget::hashChanged() const noexcept
{
	if (m_hashChanged == -1)
	{
		auto dependency = m_state.getExternalDependencyFromLocation(m_location);

		auto& sourceCache = m_state.cache.file().sources();
		auto configHash = m_state.configuration.getHash();
		auto dependencyHash = dependency != nullptr ? dependency->getHash() : std::string();
		bool cacheChanged = sourceCache.dataCacheValueChanged(Hash::string(fmt::format("cmake.{}.{}", dependencyHash, configHash)), m_hash);
		m_hashChanged = static_cast<i32>(cacheChanged);
	}
	return m_hashChanged == 1;
}

/*****************************************************************************/
const StringList& CMakeTarget::defines() const noexcept
{
	return m_defines;
}

void CMakeTarget::addDefines(StringList&& inList)
{
	List::forEach(inList, this, &CMakeTarget::addDefine);
}

void CMakeTarget::addDefine(std::string&& inValue)
{
	List::addIfDoesNotExist(m_defines, std::move(inValue));
}

/*****************************************************************************/
const StringList& CMakeTarget::targets() const noexcept
{
	return m_targets;
}
void CMakeTarget::addTargets(StringList&& inList)
{
	List::forEach(inList, this, &CMakeTarget::addTarget);
}
void CMakeTarget::addTarget(std::string&& inValue)
{
	List::addIfDoesNotExist(m_targets, std::move(inValue));
}

/*****************************************************************************/
const std::string& CMakeTarget::buildFile() const noexcept
{
	return m_buildFile;
}

void CMakeTarget::setBuildFile(std::string&& inValue) noexcept
{
	m_buildFile = std::move(inValue);
}

/*****************************************************************************/
const std::string& CMakeTarget::toolset() const noexcept
{
	return m_toolset;
}
void CMakeTarget::setToolset(std::string&& inValue) noexcept
{
	m_toolset = std::move(inValue);
}

/*****************************************************************************/
const std::string& CMakeTarget::location() const noexcept
{
	return m_location;
}

const std::string& CMakeTarget::targetFolder() const noexcept
{
	return m_targetFolder;
}

void CMakeTarget::setLocation(std::string&& inValue) noexcept
{
	m_location = std::move(inValue);
}

/*****************************************************************************/
const std::string& CMakeTarget::runExecutable() const noexcept
{
	return m_runExecutable;
}

void CMakeTarget::setRunExecutable(std::string&& inValue) noexcept
{
	m_runExecutable = std::move(inValue);
}

/*****************************************************************************/
bool CMakeTarget::recheck() const noexcept
{
	return m_recheck;
}

void CMakeTarget::setRecheck(const bool inValue) noexcept
{
	m_recheck = inValue;
}

/*****************************************************************************/
bool CMakeTarget::rebuild() const noexcept
{
	return m_rebuild;
}
void CMakeTarget::setRebuild(const bool inValue) noexcept
{
	m_rebuild = inValue;
}

/*****************************************************************************/
bool CMakeTarget::clean() const noexcept
{
	return m_clean;
}
void CMakeTarget::setClean(const bool inValue) noexcept
{
	m_clean = inValue;
}

/*****************************************************************************/
bool CMakeTarget::install() const noexcept
{
	return m_install;
}
void CMakeTarget::setInstall(const bool inValue) noexcept
{
	m_install = inValue;
}

}
