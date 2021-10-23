/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/BundleTarget.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
BundleTarget::BundleTarget() :
	IDistTarget(DistTargetType::DistributionBundle)
{
}

/*****************************************************************************/
bool BundleTarget::initialize(const BuildState& inState)
{
	const auto& targetName = this->name();
	for (auto& dir : m_rawIncludes)
	{
		inState.paths.replaceVariablesInPath(dir, targetName);
	}
	for (auto& dir : m_excludes)
	{
		inState.paths.replaceVariablesInPath(dir, targetName);
	}

	if (!resolveIncludesFromState(inState))
		return false;

	m_includesResolved = true;

	return true;
}

/*****************************************************************************/
bool BundleTarget::validate()
{
	bool result = true;

#if defined(CHALET_WIN32)
	result &= m_windowsBundle.validate();
#elif defined(CHALET_MACOS)
	result &= m_macosBundle.validate();
#else
	result &= m_linuxBundle.validate();
#endif

	return result;
}

/*****************************************************************************/
bool BundleTarget::updateRPaths() const noexcept
{
	return m_updateRPaths;
}

void BundleTarget::setUpdateRPaths(const bool inValue) noexcept
{
	m_updateRPaths = inValue;
}

/*****************************************************************************/
const BundleLinux& BundleTarget::linuxBundle() const noexcept
{
	return m_linuxBundle;
}

void BundleTarget::setLinuxBundle(BundleLinux&& inValue)
{
	m_linuxBundle = std::move(inValue);
}

/*****************************************************************************/
const BundleMacOS& BundleTarget::macosBundle() const noexcept
{
	return m_macosBundle;
}

void BundleTarget::setMacosBundle(BundleMacOS&& inValue)
{
	m_macosBundle = std::move(inValue);
}

/*****************************************************************************/
const BundleWindows& BundleTarget::windowsBundle() const noexcept
{
	return m_windowsBundle;
}

void BundleTarget::setWindowsBundle(BundleWindows&& inValue)
{
	m_windowsBundle = std::move(inValue);
}

/*****************************************************************************/
const std::string& BundleTarget::subDirectory() const noexcept
{
	return m_subDirectory;
}

void BundleTarget::setSubDirectory(std::string&& inValue)
{
	m_subDirectory = std::move(inValue);
	Path::sanitize(m_subDirectory);
}

/*****************************************************************************/
const std::string& BundleTarget::configuration() const noexcept
{
	return m_configuration;
}

void BundleTarget::setConfiguration(std::string&& inValue)
{
	m_configuration = std::move(inValue);
}

/*****************************************************************************/
const std::string& BundleTarget::mainExecutable() const noexcept
{
	return m_mainExecutable;
}

void BundleTarget::setMainExecutable(std::string&& inValue)
{
	m_mainExecutable = std::move(inValue);
}

/*****************************************************************************/
bool BundleTarget::includeDependentSharedLibraries() const noexcept
{
	return m_includeDependentSharedLibraries;
}

void BundleTarget::setIncludeDependentSharedLibraries(const bool inValue) noexcept
{
	m_includeDependentSharedLibraries = inValue;
}

/*****************************************************************************/
const StringList& BundleTarget::buildTargets() const noexcept
{
	return m_buildTargets;
}

void BundleTarget::addBuildTargets(StringList&& inList)
{
	List::forEach(inList, this, &BundleTarget::addBuildTarget);
}

void BundleTarget::addBuildTarget(std::string&& inValue)
{
	Path::sanitize(inValue);
	List::addIfDoesNotExist(m_buildTargets, std::move(inValue));
}

/*****************************************************************************/
const StringList& BundleTarget::excludes() const noexcept
{
	return m_excludes;
}

void BundleTarget::addExcludes(StringList&& inList)
{
	List::forEach(inList, this, &BundleTarget::addExclude);
}

void BundleTarget::addExclude(std::string&& inValue)
{
	Path::sanitize(inValue);
	List::addIfDoesNotExist(m_excludes, std::move(inValue));
}

/*****************************************************************************/
const StringList& BundleTarget::includes() const noexcept
{
	chalet_assert(m_includesResolved, "BundleTarget includes not resolved");
	return m_includes;
}

void BundleTarget::addIncludes(StringList&& inList)
{
	List::forEach(inList, this, &BundleTarget::addInclude);
}

void BundleTarget::addInclude(std::string&& inValue)
{
	Path::sanitize(inValue);
	List::addIfDoesNotExist(m_rawIncludes, std::move(inValue));
}

/*****************************************************************************/
bool BundleTarget::resolveIncludesFromState(const BuildState& inState)
{
	const auto add = [this](std::string in) {
		Path::sanitize(in);
		List::addIfDoesNotExist(m_includes, std::move(in));
	};

	for (auto& dependency : m_rawIncludes)
	{
		if (Commands::pathExists(dependency))
		{
			add(dependency);
			continue;
		}

		/*std::string resolved = fmt::format("{}/{}", inState.paths.buildOutputDir(), inValue);
		if (Commands::pathExists(resolved))
		{
			add(resolved);
			continue;
		}*/

		std::string resolved;
		bool found = false;
		for (auto& target : inState.targets)
		{
			if (target->isProject())
			{
				auto& project = static_cast<const SourceTarget&>(*target);

				const auto& compilerConfig = inState.getCompilerConfig(project.language());
				const auto& compilerPathBin = compilerConfig.compilerPathBin();

				resolved = fmt::format("{}/{}", compilerPathBin, dependency);
				if (Commands::pathExists(resolved))
				{
					add(std::move(resolved));
					found = true;
					break;
				}

				// LOG(resolved, ' ', project.outputFile());
				if (String::contains(project.outputFile(), resolved))
				{
					add(std::move(resolved));
					found = true;
					break;
				}
			}
		}

		if (!found)
		{

			for (auto& path : inState.workspace.searchPaths())
			{
				resolved = fmt::format("{}/{}", path, dependency);
				if (Commands::pathExists(resolved))
				{
					add(std::move(resolved));
					found = true;
					break;
				}
			}
		}
		if (!found)
		{
			resolved = Commands::which(dependency);
			if (!resolved.empty())
			{
				add(std::move(resolved));
			}
			else
			{
				Diagnostic::warn("Included path '{}' for distribution target '{}' was not found.", dependency, this->name());
			}
		}
	}

	return true;
}

/*****************************************************************************/
void BundleTarget::sortIncludes()
{
	List::sort(m_includes);
}

}
