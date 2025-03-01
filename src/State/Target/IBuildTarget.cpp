/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/IBuildTarget.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

#include "State/Target/CMakeTarget.hpp"
#include "State/Target/MesonTarget.hpp"
#include "State/Target/ProcessBuildTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "State/Target/ValidationBuildTarget.hpp"

namespace chalet
{
IBuildTarget::IBuildTarget(const BuildState& inState, const BuildTargetType inType) :
	m_state(inState),
	m_type(inType)
{
}

/*****************************************************************************/
[[nodiscard]] BuildTarget IBuildTarget::make(const BuildTargetType inType, const BuildState& inState)
{
	switch (inType)
	{
		case BuildTargetType::Source:
			return std::make_unique<SourceTarget>(inState);
		case BuildTargetType::Script:
			return std::make_unique<ScriptBuildTarget>(inState);
		case BuildTargetType::SubChalet:
			return std::make_unique<SubChaletTarget>(inState);
		case BuildTargetType::CMake:
			return std::make_unique<CMakeTarget>(inState);
		case BuildTargetType::Meson:
			return std::make_unique<MesonTarget>(inState);
		case BuildTargetType::Process:
			return std::make_unique<ProcessBuildTarget>(inState);
		case BuildTargetType::Validation:
			return std::make_unique<ValidationBuildTarget>(inState);
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented BuildTargetType requested: {}", static_cast<i32>(inType));
	return nullptr;
}

/*****************************************************************************/
bool IBuildTarget::initialize()
{
	return true;
}

/*****************************************************************************/
bool IBuildTarget::resolveDependentTargets(StringList& outDepends, std::string& outPath, const char* inKey) const
{
	bool dependsOnTargets = false;
	bool dependsOnBuiltFile = false;
	if (!outDepends.empty())
	{
		const auto& buildDir = m_state.paths.buildOutputDir();
		for (auto it = outDepends.begin(); it != outDepends.end();)
		{
			auto& depends = *it;
			if (Files::pathExists(depends))
			{
				it++;
				continue;
			}

			bool startsWithBuildDir = String::startsWith(buildDir, depends);

			if (!startsWithBuildDir && depends.find_first_of("/\\") != std::string::npos)
			{
				Diagnostic::error("The target '{}' depends on a path that was not found: {}", this->name(), depends);
				return false;
			}

			if (String::equals(this->name(), depends))
			{
				Diagnostic::error("The target '{}' depends on itself. Remove it from '{}'.", this->name(), inKey);
				return false;
			}

			bool found = false;
			bool erase = true;
			for (auto& target : m_state.targets)
			{
				if (String::equals(target->name(), this->name()))
					break;

				if (String::equals(target->name(), depends))
				{
					if (target->isSources())
					{
						depends = m_state.paths.getTargetFilename(static_cast<const SourceTarget&>(*target));
						erase = depends.empty();
					}
					else if (target->isCMake())
					{
						depends = m_state.paths.getTargetFilename(static_cast<const CMakeTarget&>(*target));
						erase = depends.empty();
					}
					else if (target->isMeson())
					{
						depends = m_state.paths.getTargetFilename(static_cast<const MesonTarget&>(*target));
						erase = depends.empty();
					}
					dependsOnTargets = true;
					found = true;
					break;
				}
			}

			if (startsWithBuildDir)
			{
				// Assume it gets created somewhere during the build
				erase = false;
				dependsOnBuiltFile = true;
				found = true;
			}

			if (!found)
			{
				Diagnostic::error("The target '{}' depends on the '{}' target which either doesn't exist or sequenced later.", this->name(), depends);
				return false;
			}

			if (erase)
				it = outDepends.erase(it);
			else
				it++;
		}
	}

	if (!Files::pathExists(outPath))
	{
		auto resolved = Files::which(outPath);
		if (resolved.empty())
		{
			if (dependsOnTargets || dependsOnBuiltFile)
			{
#if defined(CHALET_WIN32)
				auto exe = Files::getPlatformExecutableExtension();
				if (!exe.empty() && !String::endsWith(exe, outPath))
				{
					outPath += exe;
				}
#endif
				outPath = Files::getCanonicalPath(outPath);
			}
			else
			{
				Diagnostic::error("The path for the target '{}' doesn't exist: {}", this->name(), outPath);
				return false;
			}
		}
		else
		{
			outPath = std::move(resolved);
		}
	}
	return true;
}

/*****************************************************************************/
bool IBuildTarget::replaceVariablesInPathList(StringList& outList) const
{
	for (auto& dir : outList)
	{
		if (!m_state.replaceVariablesInString(dir, this))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool IBuildTarget::expandGlobPatternsInList(StringList& outList, GlobMatch inSettings) const
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
BuildTargetType IBuildTarget::type() const noexcept
{
	return m_type;
}
bool IBuildTarget::isSources() const noexcept
{
	return m_type == BuildTargetType::Source;
}
bool IBuildTarget::isSubChalet() const noexcept
{
	return m_type == BuildTargetType::SubChalet;
}
bool IBuildTarget::isCMake() const noexcept
{
	return m_type == BuildTargetType::CMake;
}
bool IBuildTarget::isMeson() const noexcept
{
	return m_type == BuildTargetType::Meson;
}
bool IBuildTarget::isScript() const noexcept
{
	return m_type == BuildTargetType::Script;
}
bool IBuildTarget::isProcess() const noexcept
{
	return m_type == BuildTargetType::Process;
}
bool IBuildTarget::isValidation() const noexcept
{
	return m_type == BuildTargetType::Validation;
}

/*****************************************************************************/
const std::string& IBuildTarget::name() const noexcept
{
	return m_name;
}
void IBuildTarget::setName(const std::string& inValue) noexcept
{
	m_name = inValue;
}

/*****************************************************************************/
const std::string& IBuildTarget::outputDescription() const noexcept
{
	return m_outputDescription;
}
void IBuildTarget::setOutputDescription(std::string&& inValue) noexcept
{
	m_outputDescription = std::move(inValue);
}

/*****************************************************************************/
bool IBuildTarget::includeInBuild() const noexcept
{
	return m_includeInBuild;
}

void IBuildTarget::setIncludeInBuild(const bool inValue) noexcept
{
	m_includeInBuild &= inValue;
}

/*****************************************************************************/
bool IBuildTarget::willBuild() const noexcept
{
	return m_willBuild;
}
void IBuildTarget::setWillBuild(const bool inValue) noexcept
{
	m_willBuild = inValue;
}

}
