/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/BundleTarget.hpp"

#include "Libraries/Format.hpp"
#include "State/BuildEnvironment.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
BundleTarget::BundleTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::DistributionBundle)
{
	setIncludeInBuild(false);
}

/*****************************************************************************/
void BundleTarget::initialize()
{
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
const std::string& BundleTarget::outDir() const noexcept
{
	return m_distDir;
}

void BundleTarget::setOutDir(const std::string& inValue)
{
	m_distDir = inValue;
	Path::sanitize(m_distDir);
}

/*****************************************************************************/
const std::string& BundleTarget::configuration() const noexcept
{
	return m_configuration;
}

void BundleTarget::setConfiguration(const std::string& inValue)
{
	m_configuration = inValue;
}

/*****************************************************************************/
const std::string& BundleTarget::mainProject() const noexcept
{
	return m_mainProject;
}

void BundleTarget::setMainProject(const std::string& inValue)
{
	m_mainProject = inValue;
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
const StringList& BundleTarget::projects() const noexcept
{
	return m_projects;
}

void BundleTarget::addProjects(StringList& inList)
{
	List::forEach(inList, this, &BundleTarget::addProject);
}

void BundleTarget::addProject(std::string& inValue)
{
	Path::sanitize(inValue);
	List::addIfDoesNotExist(m_projects, std::move(inValue));
}

/*****************************************************************************/
const StringList& BundleTarget::excludes() const noexcept
{
	return m_excludes;
}

void BundleTarget::addExcludes(StringList& inList)
{
	List::forEach(inList, this, &BundleTarget::addExclude);
}

void BundleTarget::addExclude(std::string& inValue)
{
	Path::sanitize(inValue);
	List::addIfDoesNotExist(m_excludes, std::move(inValue));
}

/*****************************************************************************/
const StringList& BundleTarget::dependencies() const noexcept
{
	return m_dependencies;
}

void BundleTarget::addDependencies(StringList& inList)
{
	List::forEach(inList, this, &BundleTarget::addDependency);
}

void BundleTarget::addDependency(std::string& inValue)
{
	const auto add = [this](std::string& in) {
		Path::sanitize(in);
		List::addIfDoesNotExist(m_dependencies, std::move(in));
	};

	if (Commands::pathExists(inValue))
	{
		add(inValue);
		return;
	}

	/*std::string resolved = fmt::format("{}/{}", m_paths.buildOutputDir(), inValue);
	if (Commands::pathExists(resolved))
	{
		add(resolved);
		return;
	}*/

	std::string resolved;
	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);

			const auto& compilerConfig = m_state.compilerTools.getConfig(project.language());
			const auto& compilerPathBin = compilerConfig.compilerPathBin();

			resolved = fmt::format("{}/{}", compilerPathBin, inValue);
			if (Commands::pathExists(resolved))
			{
				add(resolved);
				return;
			}

			// LOG(resolved, ' ', project.outputFile());
			if (String::contains(project.outputFile(), resolved))
			{
				add(resolved);
				return;
			}
		}
	}

	for (auto& path : m_state.environment.path())
	{
		resolved = fmt::format("{}/{}", path, inValue);
		if (Commands::pathExists(resolved))
		{
			add(resolved);
			return;
		}
	}
}

/*****************************************************************************/
void BundleTarget::sortDependencies()
{
	List::sort(m_dependencies);
}

}
