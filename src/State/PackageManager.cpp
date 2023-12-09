/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/PackageManager.hpp"

#include "State/BuildState.hpp"
#include "State/Package/SourcePackage.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"

namespace chalet
{
/*****************************************************************************/
PackageManager::PackageManager(const BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
bool PackageManager::initialize()
{
	UNUSED(m_state);

	for (auto& [name, pkg] : m_packages)
	{
		if (!pkg->initialize())
		{
			Diagnostic::error("Error initializing the imported package: {}", name);
			return false;
		}
	}

	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<SourceTarget&>(*target);
			auto& importedPackages = project.importPackages();
			if (importedPackages.empty())
				continue;

			for (auto& name : importedPackages)
			{
				if (m_packages.find(name) == m_packages.end())
				{
					Diagnostic::error("Found unrecognized package '{}' in target: {}", name, project.name());
					return false;
				}

				auto& pkg = m_packages.at(name);

				auto& binaries = pkg->binaries();
				auto& searchDirs = pkg->searchDirs();
				auto& includeDirs = pkg->includeDirs();
				auto& libDirs = pkg->libDirs();
				auto& appleFrameworks = pkg->appleFrameworks();
				auto& appleFrameworkPaths = pkg->appleFrameworkPaths();
				auto& links = pkg->links();
				auto& staticLinks = pkg->staticLinks();
				auto& linkerOptions = pkg->linkerOptions();

				for (auto& binary : binaries)
				{
					// LOG(binary);
					UNUSED(binary);
				}

				if (!searchDirs.empty())
					m_state.workspace.addSearchPaths(StringList(searchDirs));

				if (!includeDirs.empty())
					project.addIncludeDirs(StringList(includeDirs));

				if (!libDirs.empty())
					project.addLibDirs(StringList(libDirs));

				if (!links.empty())
					project.addLinks(StringList(links));

				if (!staticLinks.empty())
					project.addStaticLinks(StringList(staticLinks));

				if (!linkerOptions.empty())
					project.addLinkerOptions(StringList(linkerOptions));

#if defined(CHALET_MACOS)
				if (!appleFrameworkPaths.empty())
					project.addAppleFrameworkPaths(StringList(appleFrameworkPaths));

				if (!appleFrameworks.empty())
					project.addAppleFrameworks(StringList(appleFrameworks));
#endif
			}
		}
	}

	// Clear packages at this point
	m_packages.clear();

	return true;
}

/*****************************************************************************/
void PackageManager::add(const std::string& inName, Ref<SourcePackage>&& inValue)
{
	m_packages.emplace(inName, std::move(inValue));
}

}
