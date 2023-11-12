/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/IAppBundler.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

#if defined(CHALET_WIN32)
	#include "Bundler/AppBundlerWindows.hpp"
#elif defined(CHALET_MACOS)
	#include "Bundler/AppBundlerMacOS.hpp"
#elif defined(CHALET_LINUX)
	#include "Bundler/AppBundlerLinux.hpp"
#endif
#include "Bundler/AppBundlerWeb.hpp"

namespace chalet
{
/*****************************************************************************/
[[nodiscard]] Unique<IAppBundler> IAppBundler::make(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap)
{
	if (inState.environment->isEmscripten())
	{
		return std::make_unique<AppBundlerWeb>(inState, inBundle, inDependencyMap);
	}
#if defined(CHALET_WIN32)
	return std::make_unique<AppBundlerWindows>(inState, inBundle, inDependencyMap);
#elif defined(CHALET_MACOS)
	return std::make_unique<AppBundlerMacOS>(inState, inBundle, inDependencyMap);
#elif defined(CHALET_LINUX)
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
bool IAppBundler::quickBundleForPlatform()
{
	return false;
}

/*****************************************************************************/
// TODO: Move this to BundleTarget
//
bool IAppBundler::getMainExecutable(std::string& outMainExecutable)
{
	auto& buildTargets = m_bundle.buildTargets();
	auto& mainExecutable = m_bundle.mainExecutable();
	std::string lastOutput;

	// Match mainExecutable if defined, otherwise get first executable
	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (!List::contains(buildTargets, project.name()))
				continue;

			if (project.isStaticLibrary())
				continue;

			lastOutput = project.outputFile();

			if (!project.isExecutable())
				continue;

			if (!mainExecutable.empty() && !String::equals(mainExecutable, project.name()))
				continue;

			// LOG("Main exec:", project.name());
			outMainExecutable = project.outputFile();
			break;
		}
	}

	if (outMainExecutable.empty())
	{
		outMainExecutable = std::move(lastOutput);
		// return false;
	}

	return !outMainExecutable.empty();
}

/*****************************************************************************/
StringList IAppBundler::getAllExecutables() const
{
	StringList ret;

	auto& buildTargets = m_bundle.buildTargets();
	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (!List::contains(buildTargets, project.name()))
				continue;

			if (!project.isExecutable())
				continue;

			ret.emplace_back(project.outputFile());
		}
	}

	return ret;
}

/*****************************************************************************/
bool IAppBundler::copyIncludedPath(const std::string& inDep, const std::string& inOutPath)
{
	if (Files::pathExists(inDep))
	{
		const auto filename = String::getPathFilename(inDep);
		if (!filename.empty())
		{
			auto outputFile = fmt::format("{}/{}", inOutPath, filename);
			if (Files::pathExists(outputFile))
				return true; // Already copied - duplicate dependency
		}

		auto dep = inDep;
		auto& cwd = workingDirectoryWithTrailingPathSeparator();
		String::replaceAll(dep, cwd, "");

		if (!Files::copy(dep, inOutPath))
		{
			Diagnostic::warn("Dependency '{}' could not be copied to: {}", filename, inOutPath);
			return false;
		}
	}
	return true;
}

/*****************************************************************************/
const std::string& IAppBundler::workingDirectoryWithTrailingPathSeparator()
{
	if (m_cwd.empty())
	{
		m_cwd = m_state.inputs.workingDirectory() + '/';
#if defined(CHALET_WIN32)
		String::capitalize(m_cwd);
#endif
	}
	return m_cwd;
}
}
