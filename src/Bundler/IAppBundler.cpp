/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/IAppBundler.hpp"

#include "State/BuildState.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

#if defined(CHALET_WIN32)
	#include "Bundler/AppBundlerWindows.hpp"
#elif defined(CHALET_MACOS)
	#include "Bundler/AppBundlerMacOS.hpp"
#elif defined(CHALET_LINUX)
	#include "Bundler/AppBundlerLinux.hpp"
#endif

namespace chalet
{
/*****************************************************************************/
[[nodiscard]] std::unique_ptr<IAppBundler> IAppBundler::make(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap, const std::string& inInputFile)
{
#if defined(CHALET_WIN32)
	UNUSED(inInputFile);
	return std::make_unique<AppBundlerWindows>(inState, inBundle, inDependencyMap);
#elif defined(CHALET_MACOS)
	return std::make_unique<AppBundlerMacOS>(inState, inBundle, inDependencyMap, inInputFile);
#elif defined(CHALET_LINUX)
	UNUSED(inInputFile);
	return std::make_unique<AppBundlerLinux>(inState, inBundle, inDependencyMap);
#else
	Diagnostic::errorAbort("Unimplemented AppBundler requested: ");
	return nullptr;
#endif
}

/*****************************************************************************/
IAppBundler::IAppBundler(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap) :
	m_state(inState),
	m_bundle(inBundle),
	m_dependencyMap(inDependencyMap)
{
}

/*****************************************************************************/
const BundleTarget& IAppBundler::bundle() const noexcept
{
	return m_bundle;
}

/*****************************************************************************/
bool IAppBundler::getMainExecutable()
{
	auto& bundleProjects = m_bundle.projects();
	auto& mainProject = m_bundle.mainProject();
	std::string lastOutput;

	// Match mainProject if defined, otherwise get first executable
	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
			if (!List::contains(bundleProjects, project.name()))
				continue;

			if (project.isStaticLibrary())
				continue;

			lastOutput = project.outputFile();

			if (!project.isExecutable())
				continue;

			if (!mainProject.empty() && !String::equals(mainProject, project.name()))
				continue;

			// LOG("Main exec:", project.name());
			m_mainExecutable = project.outputFile();
			break;
		}
	}

	if (m_mainExecutable.empty())
	{
		m_mainExecutable = std::move(lastOutput);
		// return false;
	}

	return !m_mainExecutable.empty();
}
}
