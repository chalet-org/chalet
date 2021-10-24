/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundler.hpp"

#include "Builder/ScriptRunner.hpp"
#include "Bundler/IAppBundler.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/ScriptDistTarget.hpp"
#include "State/StatePrototype.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
AppBundler::AppBundler(const CommandLineInputs& inInputs, StatePrototype& inPrototype) :
	m_inputs(inInputs),
	m_prototype(inPrototype)
{
}

/*****************************************************************************/
bool AppBundler::runBuilds()
{
	// Build all required configurations
	m_detectedArch = m_inputs.targetArchitecture().empty() ? m_inputs.hostArchitecture() : m_inputs.targetArchitecture();

	auto makeState = [&](std::string arch, const std::string& inConfig) {
		auto configName = fmt::format("{}_{}", arch, inConfig);
		if (m_states.find(configName) == m_states.end())
		{
			CommandLineInputs inputs = m_inputs;
			inputs.setBuildConfiguration(std::string(inConfig));
			inputs.setTargetArchitecture(arch);
			auto state = std::make_unique<BuildState>(std::move(inputs), m_prototype);

			m_states.emplace(configName, std::move(state));
		}
	};

	StringList arches;
	for (auto& target : m_prototype.distribution)
	{
		if (target->isDistributionBundle())
		{
			auto& bundle = static_cast<BundleTarget&>(*target);
			makeState(m_detectedArch, bundle.configuration());
		}
	}

	// BuildState* stateArchA = nullptr;
	for (auto& [config, state] : m_states)
	{
		if (!state->initialize())
			return false;

		if (!state->doBuild(Route::Build, false))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool AppBundler::run(const DistTarget& inTarget)
{
	const auto& inputFile = m_inputs.inputFile();

	if (inTarget->isDistributionBundle())
	{
		auto& bundle = static_cast<BundleTarget&>(*inTarget);

		if (!bundle.description().empty())
		{
			Output::msgTargetDescription(bundle.description(), Output::theme().header);
			Output::lineBreak();
		}

		chalet_assert(!bundle.configuration().empty(), "State not initialized");

		BuildState* buildState = nullptr;
		{
			buildState = getBuildState(bundle.configuration());
			if (buildState == nullptr)
				return false;
		}

		auto bundler = IAppBundler::make(*buildState, bundle, m_dependencyMap, inputFile);
		if (!removeOldFiles(*bundler))
		{
			Diagnostic::error("There was an error removing the previous distribution bundle for: {}", inTarget->name());
			return false;
		}

		if (!bundle.initialize(*buildState))
			return false;

		if (!gatherDependencies(bundle, *buildState))
			return false;

		if (!runBundleTarget(*bundler, *buildState))
			return false;
	}
	else if (inTarget->isScript())
	{
		Timer buildTimer;

		if (!runScriptTarget(static_cast<const ScriptDistTarget&>(*inTarget), inputFile))
			return false;

		auto res = buildTimer.stop();
		if (res > 0 && Output::showBenchmarks())
		{
			Output::printInfo(fmt::format("   Time: {}", buildTimer.asString()));
		}
	}

	Output::lineBreak();

	return true;
}

/*****************************************************************************/
BuildState* AppBundler::getBuildState(const std::string& inBuildConfiguration) const
{
	BuildState* ret = nullptr;
	for (auto& [config, state] : m_states)
	{
		auto configName = fmt::format("{}_{}", m_detectedArch, inBuildConfiguration);
		if (String::equals(configName, config))
		{
			ret = state.get();
			break;
		}
	}

	chalet_assert(ret != nullptr, "State not initialized");
	if (ret == nullptr)
	{
		Diagnostic::error("Arch and/or build configuration '{}' not detected.", inBuildConfiguration);
		return nullptr;
	}

	return ret;
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
	const auto& buildTargets = inBundler.bundle().buildTargets();

	const auto bundlePath = inBundler.getBundlePath();
	const auto executablePath = inBundler.getExecutablePath();
	const auto resourcePath = inBundler.getResourcePath();

	makeBundlePath(bundlePath, executablePath, resourcePath);

	// Timer timer;

	const auto cwd = m_inputs.workingDirectory() + '/';

	const auto copyIncludedPath = [&cwd](const std::string& inDep, const std::string& inOutPath) -> bool {
		if (Commands::pathExists(inDep))
		{
			const auto filename = String::getPathFilename(inDep);
			if (!filename.empty())
			{
				auto outputFile = fmt::format("{}/{}", inOutPath, filename);
				if (Commands::pathExists(outputFile))
					return true; // Already copied - duplicate dependency
			}

			auto dep = inDep;
			String::replaceAll(dep, cwd, "");

			if (!Commands::copy(dep, inOutPath))
			{
				Diagnostic::warn("Dependency '{}' could not be copied to: {}", filename, inOutPath);
				return false;
			}
		}
		return true;
	};

	for (auto& dep : bundle.includes())
	{
#if defined(CHALET_MACOS)
		if (!String::endsWith(".framework", dep))
		{
			if (String::endsWith(".dylib", dep))
			{
				if (!copyIncludedPath(dep, executablePath))
					return false;

				// auto filename = String::getPathFilename(dep);
				// auto dylib = fmt::format("{}/{}", executablePath);
			}
			else
			{
				if (!copyIncludedPath(dep, resourcePath))
					return false;
			}
		}
#else
		if (!copyIncludedPath(dep, resourcePath))
			return false;
#endif
	}

	StringList executables;
	StringList dependenciesToCopy;
	uint copyCount = 0;
	for (auto& target : inState.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const SourceTarget&>(*target);

			if (!List::contains(buildTargets, project.name()))
				continue;

			const auto& outputFile = project.outputFile();
			const auto outputFilePath = fmt::format("{}/{}", buildOutputDir, outputFile);

			if (project.isStaticLibrary())
			{
				List::addIfDoesNotExist(dependenciesToCopy, outputFilePath);
				continue;
			}
			else if (project.isExecutable())
			{
				std::string outTarget = outputFilePath;
				List::addIfDoesNotExist(executables, std::move(outTarget));
			}

			auto dependencies = m_dependencyMap.find(outputFilePath);
			if (dependencies == m_dependencyMap.end())
			{
				Diagnostic::error("Dependency not cached: {}", outputFilePath);
				return false;
			}

			if (!copyIncludedPath(outputFilePath, executablePath))
				continue;

			for (auto& dep : dependencies->second)
			{
				List::addIfDoesNotExist(dependenciesToCopy, dep);

				auto depsOfDep = m_dependencyMap.find(dep);
				if (depsOfDep != m_dependencyMap.end())
				{
					for (auto& d : depsOfDep->second)
					{
						List::addIfDoesNotExist(dependenciesToCopy, d);
					}
				}
			}
		}
	}

	std::sort(dependenciesToCopy.begin(), dependenciesToCopy.end());
	for (auto& dep : dependenciesToCopy)
	{
#if defined(CHALET_MACOS)
		if (!String::endsWith(".framework", dep))
#endif
		{
			if (!copyIncludedPath(dep, executablePath))
				continue;
		}

		++copyCount;
	}

#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
	for (auto& exec : executables)
	{
		const auto filename = String::getPathFilename(exec);
		const auto executable = fmt::format("{}/{}", executablePath, filename);

		if (!Commands::setExecutableFlag(executable))
		{
			Diagnostic::warn("Exececutable flag could not be set for: {}", executable);
			continue;
		}
	}
#endif

	// LOG("Distribution dependencies gathered in:", timer.asString());

	Commands::forEachGlobMatch(resourcePath, bundle.excludes(), GlobMatch::FilesAndFolders, [&](const fs::path& inPath) {
		Commands::remove(inPath.string());
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

/*****************************************************************************/
bool AppBundler::gatherDependencies(const BundleTarget& inTarget, BuildState& inState)
{
	const auto& buildTargets = inTarget.buildTargets();
	const auto& buildOutputDir = inState.paths.buildOutputDir();

	StringList depsFromJson;
	for (auto& dep : inTarget.includes())
	{
		if (!Commands::pathExists(dep))
			continue;

		depsFromJson.push_back(dep);
	}

	StringList searchDirs;
	for (auto& target : inState.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			for (auto& dir : project.libDirs())
			{
				List::addIfDoesNotExist(searchDirs, dir);
			}
		}
	}

	if (inTarget.includeDependentSharedLibraries())
	{
		StringList allDependencies;

		for (auto& target : inState.targets)
		{
			if (target->isProject())
			{
				auto& project = static_cast<const SourceTarget&>(*target);

				if (List::contains(buildTargets, project.name()))
				{
					const auto& outputFile = project.outputFile();
					auto outputFilePath = fmt::format("{}/{}", buildOutputDir, outputFile);
					if (!project.isStaticLibrary())
					{
						List::addIfDoesNotExist(allDependencies, std::move(outputFilePath));
					}
				}
			}
		}

#if defined(CHALET_MACOS)
		for (auto& dep : inTarget.includes())
		{
			if (String::endsWith(".dylib", dep))
			{
				List::addIfDoesNotExist(allDependencies, dep);
			}
		}
#endif

		// TODO: Refactor this w/ recursion

		for (auto& outputFilePath : allDependencies)
		{
			if (m_dependencyMap.find(outputFilePath) != m_dependencyMap.end())
				continue;

			StringList dependencies;
			if (!inState.tools.getExecutableDependencies(outputFilePath, dependencies))
			{
				Diagnostic::error("Dependencies not found for file: '{}'", outputFilePath);
				return false;
			}

			for (auto& dep : dependencies)
			{
				const auto filename = String::getPathFilename(dep);
				if (dep.empty()
					|| List::contains(depsFromJson, dep)
					|| List::contains(depsFromJson, filename))
					continue;

				if (!Commands::pathExists(dep))
				{
					std::string resolved;
					for (auto& dir : searchDirs)
					{
						resolved = fmt::format("{}/{}", dir, filename);
						if (!Commands::pathExists(resolved))
						{
							resolved.clear();
							continue;
						}
						else
						{
							dep = std::move(resolved);
							break;
						}
					}

					if (resolved.empty())
					{
						resolved = Commands::which(filename);
						if (resolved.empty())
						{
							// Diagnostic::warn("Dependency not copied (not found in path): '{}'", dep);
							// return false;
							// We probably don't care about them anyway
							continue;
						}
						dep = std::move(resolved);
					}
				}

				if (m_dependencyMap.find(dep) != m_dependencyMap.end())
					continue;

				StringList depsOfDeps;
				if (!inState.tools.getExecutableDependencies(dep, depsOfDeps))
				{
					Diagnostic::error("Dependencies not found for file: '{}'", dep);
					return false;
				}

				for (auto& d : depsOfDeps)
				{
					auto file = String::getPathFilename(d);
					if (d.empty()
						|| List::contains(depsFromJson, d)
						|| List::contains(depsFromJson, file))
						continue;

					if (!Commands::pathExists(d))
					{
						std::string resolved;
						for (auto& dir : searchDirs)
						{
							resolved = fmt::format("{}/{}", dir, file);
							if (!Commands::pathExists(resolved))
							{
								resolved.clear();
								continue;
							}
							else
							{
								d = std::move(resolved);
								break;
							}
						}

						if (resolved.empty())
						{
							resolved = Commands::which(file);
							if (resolved.empty())
							{
								// Diagnostic::warn("Dependency not copied (not found in path): '{}'", d);
								// return false;
								// We probably don't care about them anyway
								continue;
							}
							d = std::move(resolved);
						}
					}
				}
				m_dependencyMap.emplace(dep, std::move(depsOfDeps));
			}

			m_dependencyMap.emplace(std::move(outputFilePath), std::move(dependencies));
		}
	}

	// logDependencies();

	return true;
}

/*****************************************************************************/
bool AppBundler::runScriptTarget(const ScriptDistTarget& inScript, const std::string& inInputFile)
{
	const auto& scripts = inScript.scripts();
	if (scripts.empty())
		return false;

	if (!inScript.description().empty())
		Output::msgTargetDescription(inScript.description(), Output::theme().header);
	else
		Output::msgScript(inScript.name(), Output::theme().header);

	Output::lineBreak();

	ScriptRunner scriptRunner(m_inputs, m_prototype.tools, inInputFile);
	bool showExitCode = false;
	if (!scriptRunner.run(scripts, showExitCode))
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
	const auto& bundle = inBundler.bundle();
	const auto& subDirectory = bundle.subDirectory();

	if (!List::contains(m_removedDirs, subDirectory))
	{
		Commands::removeRecursively(subDirectory);
		m_removedDirs.push_back(subDirectory);
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

		if (!Commands::makeDirectory(dir))
			return false;
	}

	return true;
}
}
