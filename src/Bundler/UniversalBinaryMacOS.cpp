/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/UniversalBinaryMacOS.hpp"

#include "Bundler/AppBundlerMacOS.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "State/CacheTools.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
UniversalBinaryMacOS::UniversalBinaryMacOS(const CommandLineInputs& inInputs, AppBundler& inBundler, BuildState& inState, const bool inInstallDependencies) :
	m_inputs(inInputs),
	m_bundler(inBundler),
	m_state(inState),
	m_installDependencies(inInstallDependencies)
{
}

/*****************************************************************************/
bool UniversalBinaryMacOS::createOppositeState()
{
	m_oppositeState = getIntermediateState("arm64", "x86_64");
	if (m_oppositeState == nullptr)
	{
		Diagnostic::error("Universal Binary builder expects 'x86_64' or 'arm64' architecture.");
		return false;
	}

	return true;
}

/*****************************************************************************/
bool UniversalBinaryMacOS::buildOppositeState()
{
	chalet_assert(m_oppositeState != nullptr, "");
	if (!m_oppositeState->doBuild(Route::Build))
		return false;

	return true;
}

/*****************************************************************************/
bool UniversalBinaryMacOS::run()
{
	if (m_state.tools.lipo().empty())
	{
		Diagnostic::error("The tool 'lipo' was not found in PATH, but is required for universal bundles.");
		return false;
	}

	chalet_assert(m_oppositeState != nullptr, "");

	auto quiet = Output::quietNonBuild();
	Output::setQuietNonBuild(true);

	auto universalState = getIntermediateState("universal", "universal");
	if (universalState == nullptr)
	{
		Diagnostic::error("Universal Binary builder expects 'x86_64' or 'arm64' architecture.");
		return false;
	}

	Output::setQuietNonBuild(quiet);

	if (!gatherDependencies(m_state, *m_oppositeState, *universalState))
		return false;

	if (!createUniversalBinaries(m_state, *m_oppositeState, *universalState))
		return false;

	return bundleState(*universalState);
}

/*****************************************************************************/
bool UniversalBinaryMacOS::gatherDependencies(BuildState& inStateA, BuildState& inStateB, BuildState& inUniversalState)
{
	auto gather = [&](BuildState& inState) -> bool {
		for (auto& target : inState.distribution)
		{
			if (target->isDistributionBundle())
			{
				auto& bundle = static_cast<BundleTarget&>(*target);
				bundle.setUpdateRPaths(false);

				if (!m_bundler.gatherDependencies(bundle, inState))
					return false;
			}
		}
		return true;
	};

	if (!gather(inStateA))
		return false;

	if (!gather(inStateB))
		return false;

	for (auto& target : inUniversalState.distribution)
	{
		if (target->isDistributionBundle())
		{
			auto& bundle = static_cast<BundleTarget&>(*target);
			bundle.setUpdateRPaths(false);
		}
	}

	BinaryDependencyMap newDeps;
	auto& archABuildDir = inStateA.paths.buildOutputDir();
	auto& universalBuildDir = inUniversalState.paths.buildOutputDir();
	for (auto& [file, deps] : m_bundler.dependencyMap())
	{
		if (String::contains(archABuildDir, file))
		{
			StringList outDeps;
			for (std::string dep : deps)
			{
				String::replaceAll(dep, archABuildDir, universalBuildDir);
				outDeps.push_back(std::move(dep));
			}
			newDeps.emplace(file, std::move(outDeps));
		}
	}

	for (auto& [file, deps] : newDeps)
	{
		auto outFile = file;
		String::replaceAll(outFile, archABuildDir, universalBuildDir);
		m_bundler.addDependencies(std::move(outFile), std::move(deps));
	}

	// m_bundler.logDependencies();

	return true;
}

/*****************************************************************************/
std::unique_ptr<BuildState> UniversalBinaryMacOS::getIntermediateState(std::string_view inReplaceA, std::string_view inReplaceB) const
{
	std::string arch = m_state.info.targetArchitectureString();

	if (String::startsWith("x86_64-", arch))
		String::replaceAll(arch, "x86_64", inReplaceA);
	else if (String::startsWith("arm64-", arch))
		String::replaceAll(arch, "arm64", inReplaceB);
	else
		return nullptr;

	CommandLineInputs inputs = m_inputs;
	inputs.setTargetArchitecture(std::move(arch));

	UNUSED(m_installDependencies);

	// auto buildState = std::make_unique<BuildState>(inputs);
	// if (!buildState->initialize(m_installDependencies))
	// 	return nullptr;

	// return buildState;
	return nullptr;
}

/*****************************************************************************/
StringList UniversalBinaryMacOS::getProjectFiles(const BuildState& inState) const
{
	StringList ret;

	auto& buildOutputDir = inState.paths.buildOutputDir();
	for (auto& target : inState.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<ProjectTarget&>(*target);
			if (project.isStaticLibrary())
				continue;

			ret.push_back(fmt::format("{}/{}", buildOutputDir, project.outputFile()));
		}
	}

	return ret;
}

/*****************************************************************************/
// Reference: https://developer.apple.com/documentation/apple-silicon/building-a-universal-macos-binary
bool UniversalBinaryMacOS::createUniversalBinaries(const BuildState& inStateA, const BuildState& inStateB, const BuildState& inUniversalState) const
{
	StringList outputFilesA = getProjectFiles(inStateA);
	StringList outputFilesB = getProjectFiles(inStateB);
	StringList outputFilesUniversal = getProjectFiles(inUniversalState);

	chalet_assert(outputFilesA.size() == outputFilesB.size() && outputFilesA.size() == outputFilesUniversal.size(), "");

	auto& universalBuildDir = inUniversalState.paths.buildOutputDir();
	if (!Commands::pathExists(universalBuildDir))
	{
		if (!Commands::makeDirectory(universalBuildDir))
		{
			Diagnostic::error(fmt::format("There was an error creating the directory: {}", universalBuildDir));
			return false;
		}
	}

	auto& archA = inStateA.info.targetArchitectureString();
	auto& archB = inStateB.info.targetArchitectureString();

	auto& installNameTool = inUniversalState.tools.installNameTool();

	auto makeIntermediateFile = [&](const std::string& inFile, const std::string& inArch, const std::string& inOutFolder) {
		auto tmpFolder = fmt::format("{}/tmp_{}", inOutFolder, inArch);
		if (!Commands::pathExists(tmpFolder))
			Commands::makeDirectory(tmpFolder);

		auto file = String::getPathFilename(inFile);
		auto tmpFile = fmt::format("{}/{}", tmpFolder, file);
		if (Commands::copySilent(inFile, tmpFolder))
		{
			auto iter = m_bundler.dependencyMap().find(inFile);
			if (!AppBundlerMacOS::changeRPathOfDependents(installNameTool, file, iter->second, tmpFile, true))
			{
				Diagnostic::error("Error chaning run path for file: {}", tmpFile);
				return std::make_pair(std::string(), std::string());
			}
		}
		return std::make_pair(std::move(tmpFile), std::move(tmpFolder));
	};

	StringList removeFolders;
	for (std::size_t i = 0; i < outputFilesA.size(); ++i)
	{
		auto& fileUniversal = outputFilesUniversal[i];

		auto outFolder = String::getPathFolder(fileUniversal);

		auto [tmpFileA, tmpFolderA] = makeIntermediateFile(outputFilesA[i], archA, outFolder);
		if (tmpFileA.empty())
			return false;

		auto [tmpFileB, tmpFolderB] = makeIntermediateFile(outputFilesB[i], archB, outFolder);
		if (tmpFileA.empty())
			return false;

		// LOG(fileArchA, fileArchB, fileUniversal);

		// Example:
		// lipo -create -output universal-apple-darwin_Release/chalet x86_64-apple-darwin_Release/chalet  arm64-apple-darwin_Release/chalet

		if (!Commands::subprocess({ m_state.tools.lipo(), "-create", "-output", fileUniversal, std::move(tmpFileA), std::move(tmpFileB) }))
		{
			Diagnostic::error(fmt::format("There was an error making the binary: {}", fileUniversal));
			return false;
		}

		List::addIfDoesNotExist(removeFolders, std::move(tmpFolderA));
		List::addIfDoesNotExist(removeFolders, std::move(tmpFolderB));
	}

	for (auto& path : removeFolders)
	{
		Commands::removeRecursively(path);
	}

	return true;
}

/*****************************************************************************/
bool UniversalBinaryMacOS::bundleState(BuildState& inUniversalState)
{
	for (auto& target : m_state.distribution)
	{
		if (target->isDistributionBundle())
		{
			auto& bundle = static_cast<BundleTarget&>(*target);
			if (!bundle.macosBundle().universalBinary())
			{
				if (!m_bundler.run(target))
					return false;
			}
		}
	}

	for (auto& target : inUniversalState.distribution)
	{
		if (target->isDistributionBundle())
		{
			auto& bundle = static_cast<BundleTarget&>(*target);
			if (bundle.macosBundle().universalBinary())
			{
				if (!m_bundler.run(target))
					return false;
			}
		}
	}

	return true;
}
}
