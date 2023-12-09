/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/PackageManager.hpp"

#include "ChaletJson/ChaletJsonParser.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "State/Package/SourcePackage.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
PackageManager::PackageManager(BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
bool PackageManager::initialize()
{
	if (!resolvePackagesFromSubChaletTargets())
		return false;

	if (!initializePackages())
		return false;

	if (!readImportedPackages())
		return false;

	// Clear packages at this point. in theory, we should no longer need them
	m_packages.clear();

	return true;
}

/*****************************************************************************/
bool PackageManager::add(const std::string& inName, Ref<SourcePackage>&& inValue)
{
	if (m_packages.find(inName) != m_packages.end())
	{
		Diagnostic::error("Duplicate package name was found: {}", inName);
		return false;
	}

	m_packages.emplace(inName, std::move(inValue));
	return true;
}

/*****************************************************************************/
const StringList& PackageManager::packagePaths() const noexcept
{
	return m_packagePaths;
}
void PackageManager::addPackagePaths(StringList&& inList)
{
	List::forEach(inList, this, &PackageManager::addPackagePath);
}
void PackageManager::addPackagePath(std::string&& inValue)
{
	List::addIfDoesNotExist(m_packagePaths, std::move(inValue));
}

/*****************************************************************************/
bool PackageManager::resolvePackagesFromSubChaletTargets()
{
	Unique<ChaletJsonParser> chaletJsonParser;

	auto packages = std::move(m_packages);
	m_packages.clear();

	auto readPackagesIfAvailable = [this, &chaletJsonParser](const std::string& location, std::string buildFile) {
		if (buildFile.empty())
			buildFile = m_state.inputs.defaultInputFile();

		auto resolved = fmt::format("{}/{}", location, buildFile);
		if (!Files::pathExists(resolved))
		{
			auto base = String::getPathFolderBaseName(buildFile);
			resolved = fmt::format("{}/{}.yaml", location, base);
		}

		if (!Files::pathExists(resolved))
			return true;

		if (!chaletJsonParser)
		{
			chaletJsonParser = std::make_unique<ChaletJsonParser>(m_state);
		}

		if (!chaletJsonParser->readPackagesIfAvailable(resolved, location))
		{
			Diagnostic::error("Error importing packages from: {}", resolved);
			return false;
		}

		return true;
	};

	for (auto& path : m_packagePaths)
	{
		if (!m_state.replaceVariablesInString(path, static_cast<const SourcePackage*>(nullptr)))
			return false;

		auto location = path;
		if (!Files::pathIsDirectory(location))
			location = String::getPathFolder(path);

		std::string buildFile;
		if (String::endsWith({ ".json", ".yaml" }, path))
			buildFile = String::getPathFilename(path);

		if (!readPackagesIfAvailable(location, buildFile))
			return false;
	}

	for (auto& target : m_state.targets)
	{
		if (target->isSubChalet())
		{
			auto& project = static_cast<SubChaletTarget&>(*target);
			auto& location = project.location();

			if (!readPackagesIfAvailable(location, project.buildFile()))
				return false;
		}
	}

	for (auto&& pkg : packages)
	{
		if (!add(pkg.first, std::move(pkg.second)))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool PackageManager::initializePackages()
{
	auto cwd = Files::getWorkingDirectory();
	auto onError = [&cwd]() {
		Files::changeWorkingDirectory(cwd);
		return false;
	};

	for (auto& [name, pkg] : m_packages)
	{
		bool rootChanged = false;
		auto& root = pkg->root();
		if (!root.empty())
		{
			if (!Files::pathExists(root))
			{
				Diagnostic::error("Error resolving the path to the imported package: {}", name);
				return false;
			}

			Files::changeWorkingDirectory(root);
			rootChanged = true;
		}

		// LOG(name, "--", pkg->root());

		// Note: slow
		if (!pkg->initialize())
		{
			Diagnostic::error("Error initializing the imported package: {}", name);
			return onError();
		}

		if (rootChanged)
			Files::changeWorkingDirectory(cwd);
	}

	return true;
}

/*****************************************************************************/
bool PackageManager::readImportedPackages()
{
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

				auto& searchPaths = pkg->searchPaths();
				auto& includeDirs = pkg->includeDirs();
				auto& libDirs = pkg->libDirs();
				auto& appleFrameworks = pkg->appleFrameworks();
				auto& appleFrameworkPaths = pkg->appleFrameworkPaths();
				auto& links = pkg->links();
				auto& staticLinks = pkg->staticLinks();
				auto& linkerOptions = pkg->linkerOptions();

				if (!searchPaths.empty())
					m_state.workspace.addSearchPaths(StringList(searchPaths));

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

	return true;
}
}
