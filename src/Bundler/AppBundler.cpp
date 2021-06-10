/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundler.hpp"

#include "Builder/ScriptRunner.hpp"
#include "Bundler/IAppBundler.hpp"
#include "Libraries/Format.hpp"
#include "State/BuildState.hpp"
#include "State/Target/BundleTarget.hpp"
#include "State/Target/ScriptTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{

/*****************************************************************************/
bool AppBundler::run(BuildTarget& inTarget, BuildState& inState, const std::string& inBuildFile)
{
	m_cleanOutput = inState.environment.cleanOutput();

	if (inTarget->isDistributionBundle())
	{
		auto& bundle = static_cast<BundleTarget&>(*inTarget);
		auto bundler = IAppBundler::make(inState, bundle, m_dependencyMap, inBuildFile, m_cleanOutput);
		if (!removeOldFiles(*bundler))
		{
			Diagnostic::error(fmt::format("There was an error removing the previous distribution bundle for: {}", inTarget->name()));
			return false;
		}

		if (!gatherDependencies(bundle, inState))
			return false;

		if (!runBundleTarget(*bundler, inState))
			return false;
	}
	else if (inTarget->isScript())
	{
		Timer buildTimer;

		if (!runScriptTarget(static_cast<const ScriptTarget&>(*inTarget), inState, inBuildFile))
			return false;

		Output::print(Color::Reset, fmt::format("   Time: {}", buildTimer.asString()));
		Output::lineBreak();
	}

	// logDependencies();

	return true;
}

/*****************************************************************************/
void AppBundler::logDependencies() const
{
	for (auto& [file, dependencies] : m_dependencyMap)
	{
		LOG(file);
		for (auto& dep : dependencies)
		{
			LOG("    ", dep);
		}
	}
	LOG("");
}

/*****************************************************************************/
bool AppBundler::runBundleTarget(IAppBundler& inBundler, BuildState& inState)
{
	auto& bundle = inBundler.bundle();
	const auto& buildOutputDir = inState.paths.buildOutputDir();
	const auto& bundleProjects = inBundler.bundle().projects();

	const auto bundlePath = inBundler.getBundlePath();
	const auto executablePath = inBundler.getExecutablePath();
	const auto resourcePath = inBundler.getResourcePath();

	makeBundlePath(bundlePath, executablePath, resourcePath);

	// auto path = Environment::getPath();
	// LOG(path);

	// Timer timer;

	const auto copyDependency = [cleanOutput = m_cleanOutput](const std::string& inDep, const std::string& inOutPath) -> bool {
		if (Commands::pathExists(inDep))
		{
			const auto filename = String::getPathFilename(inDep);
			if (!filename.empty())
			{
				auto outputFile = fmt::format("{}/{}", inOutPath, filename);
				if (Commands::pathExists(outputFile))
					return true; // Already copied - duplicate dependency
			}
			if (!Commands::copy(inDep, inOutPath, cleanOutput))
			{
				Diagnostic::warn(fmt::format("Dependency '{}' could not be copied to: {}", filename, inOutPath));
				return false;
			}
		}
		return true;
	};

	for (auto& dep : bundle.dependencies())
	{
		if (!copyDependency(dep, resourcePath))
			return false;
	}

	StringList executables;
	uint copyCount = 0;
	for (auto& target : inState.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);

			if (!List::contains(bundleProjects, project.name()))
				continue;

			const auto& outputFile = project.outputFile();
			const auto outputFilePath = fmt::format("{}/{}", buildOutputDir, outputFile);

			if (project.isExecutable())
			{
				std::string outTarget = outputFilePath;
				List::addIfDoesNotExist(executables, std::move(outTarget));
			}

			auto dependencies = m_dependencyMap.find(outputFilePath);
			if (dependencies == m_dependencyMap.end())
			{
				Diagnostic::error(fmt::format("Dependency not cached: {}", outputFilePath));
				return false;
			}

			if (!copyDependency(outputFilePath, executablePath))
				continue;

			for (auto& dep : dependencies->second)
			{
				if (!copyDependency(dep, executablePath))
					continue;

				++copyCount;
			}
		}
	}

#if !defined(CHALET_WIN32)
	for (auto& exec : executables)
	{
		const auto filename = String::getPathFilename(exec);
		const auto executable = fmt::format("{}/{}", executablePath, filename);

		if (!Commands::setExecutableFlag(executable, m_cleanOutput))
		{
			Diagnostic::warn(fmt::format("Exececutable flag could not be set for: {}", executable));
			continue;
		}
	}
#endif

	// LOG("Distribution dependencies gathered in:", timer.asString());

	Commands::forEachFileMatch(resourcePath, bundle.excludes(), [this](const fs::path& inPath) {
		Commands::remove(inPath.string(), m_cleanOutput);
	});

	if (copyCount > 0)
	{
		// Output::lineBreak();
	}

	if (!inBundler.bundleForPlatform())
		return false;

	return true;
}

/*****************************************************************************/
const BinaryDependencyMap& AppBundler::dependencyMap() const noexcept
{
	return m_dependencyMap;
}

bool AppBundler::gatherDependencies(BundleTarget& inTarget, BuildState& inState)
{
	const auto& bundleProjects = inTarget.projects();
	const auto& buildOutputDir = inState.paths.buildOutputDir();

	StringList depsFromJson;
	for (auto& dep : inTarget.dependencies())
	{
		if (!Commands::pathExists(dep))
			continue;

		depsFromJson.push_back(dep);
	}

	/*StringList projectNames;
	for (auto& target : inState.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
			projectNames.push_back(String::getPathFilename(project.outputFile()));
		}
	}*/

	for (auto& target : inState.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);

			if (List::contains(bundleProjects, project.name()))
			{
				const auto& outputFile = project.outputFile();
				const auto outputFilePath = fmt::format("{}/{}", buildOutputDir, outputFile);

				// dependencies.push_back(outputFilePath);

				if (inTarget.includeDependentSharedLibraries())
				{
					if (m_dependencyMap.find(outputFilePath) == m_dependencyMap.end())
					{
						StringList dependencies;
						if (!project.isStaticLibrary())
						{
							if (!inState.tools.getExecutableDependencies(outputFilePath, dependencies))
							{
								Diagnostic::error(fmt::format("Dependencies not found for file: '{}'", outputFilePath));
								return false;
							}

							for (auto& dep : dependencies)
							{
								const auto filename = String::getPathFilename(dep);
								if (dep.empty()
									|| List::contains(depsFromJson, dep)
									|| List::contains(depsFromJson, filename)
									//  || List::contains(projectNames, dep)
								)
									continue;

								std::string depPath;
								if (!Commands::pathExists(dep))
								{
									depPath = Commands::which(dep);
									if (depPath.empty())
									{
										Diagnostic::error(fmt::format("Dependency not found in path: '{}'", dep));
										return false;
									}
								}
								else
								{
									depPath = dep;
								}

								if (m_dependencyMap.find(depPath) == m_dependencyMap.end())
								{
									StringList depsOfDeps;
									if (!inState.tools.getExecutableDependencies(depPath, depsOfDeps))
									{
										Diagnostic::error(fmt::format("Dependencies not found for file: '{}'", depPath));
										return false;
									}
									m_dependencyMap.emplace(std::move(depPath), std::move(depsOfDeps));
								}
							}
						}

						m_dependencyMap.emplace(std::move(outputFilePath), std::move(dependencies));
					}
				}
			}
		}
	}

	return true;
}

/*****************************************************************************/
void AppBundler::addDependencies(std::string&& inFile, StringList&& inDependencies)
{
	if (m_dependencyMap.find(inFile) == m_dependencyMap.end())
	{
		m_dependencyMap.emplace(std::move(inFile), std::move(inDependencies));
	}
}

/*****************************************************************************/
bool AppBundler::runScriptTarget(const ScriptTarget& inScript, BuildState& inState, const std::string& inBuildFile)
{
	const auto& scripts = inScript.scripts();
	if (scripts.empty())
		return false;

	if (!inScript.description().empty())
		Output::msgScriptDescription(inScript.description());
	else
		Output::msgScript(inScript.name());

	Output::lineBreak();

	ScriptRunner scriptRunner(inState.tools, inBuildFile, m_cleanOutput);
	if (!scriptRunner.run(scripts))
	{
		Output::lineBreak();
		Output::msgBuildFail(); // TODO: Script failed
		Output::lineBreak();
		return false;
	}

	return true;
}

/*****************************************************************************/
bool AppBundler::removeOldFiles(IAppBundler& inBundler)
{
	auto& bundle = inBundler.bundle();
	const auto& outDir = bundle.outDir();

	if (!List::contains(m_removedDirs, outDir))
	{
		Commands::removeRecursively(outDir, m_cleanOutput);
		m_removedDirs.push_back(outDir);
	}

	if (!inBundler.removeOldFiles())
		return false;

	return true;
}

/*****************************************************************************/
bool AppBundler::makeBundlePath(const std::string& inBundlePath, const std::string& inExecutablePath, const std::string& inResourcePath)
{
	StringList dirList{ inBundlePath };
	List::addIfDoesNotExist(dirList, inExecutablePath);
	List::addIfDoesNotExist(dirList, inResourcePath);

	// make prod dir
	for (auto& dir : dirList)
	{
		if (Commands::pathExists(dir))
			continue;

		if (!Commands::makeDirectory(dir, m_cleanOutput))
			return false;
	}

	return true;
}
}
