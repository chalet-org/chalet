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
UniversalBinaryMacOS::UniversalBinaryMacOS(AppBundler& inBundler, BuildState& inState, BundleTarget& inBundle) :
	m_bundler(inBundler),
	m_state(inState),
	m_bundle(inBundle)
{
}

/*****************************************************************************/
bool UniversalBinaryMacOS::run(BuildState& inStateB, BuildState& inUniversalState)
{
	if (m_state.tools.lipo().empty())
	{
		Diagnostic::error("The tool 'lipo' was not found in PATH, but is required for universal bundles.");
		return false;
	}

	if (!gatherDependencies(m_state, inStateB, inUniversalState))
		return false;

	if (!createUniversalBinaries(m_state, inStateB, inUniversalState))
		return false;

	return true;
}

/*****************************************************************************/
bool UniversalBinaryMacOS::gatherDependencies(BuildState& inStateA, BuildState& inStateB, BuildState& inUniversalState)
{
	auto gather = [&](BuildState& inState) -> bool {
		m_bundle.setUpdateRPaths(false);
		m_bundle.initialize(inState);

		if (!m_bundler.gatherDependencies(m_bundle, inState))
			return false;

		return true;
	};

	if (!gather(inStateA))
		return false;

	if (!gather(inStateB))
		return false;

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

			if (List::contains(m_bundle.projects(), project.name()))
			{
				ret.push_back(fmt::format("{}/{}", buildOutputDir, project.outputFile()));
			}
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

	auto dependencyMap = m_bundler.dependencyMap();

	auto makeIntermediateFile = [&](const std::string& inFile, const std::string& inArch, const std::string& inOutFolder) {
		auto tmpFolder = fmt::format("{}/tmp_{}", inOutFolder, inArch);
		if (!Commands::pathExists(tmpFolder))
			Commands::makeDirectory(tmpFolder);

		auto file = String::getPathFilename(inFile);
		auto tmpFile = fmt::format("{}/{}", tmpFolder, file);
		if (Commands::copySilent(inFile, tmpFolder))
		{
			auto iter = dependencyMap.find(inFile);
			if (iter == dependencyMap.end())
			{
				Diagnostic::error(fmt::format("There was an error finding the file: {}", inFile));
				return std::make_pair(std::string(), std::string());
			}
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
}
