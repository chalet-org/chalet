/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/UniversalBinaryMacOS.hpp"

#include "Bundler/AppBundler.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
UniversalBinaryMacOS::UniversalBinaryMacOS(const CommandLineInputs& inInputs, BuildState& inState, const bool inInstallDependencies) :
	m_inputs(inInputs),
	m_state(inState),
	m_installDependencies(inInstallDependencies)
{
}

/*****************************************************************************/
bool UniversalBinaryMacOS::run()
{
	if (m_state.tools.lipo().empty())
	{
		Diagnostic::error("The tool 'lipo' was not found in PATH, but is required for universal bundles.");
		return false;
	}

	auto oppositeState = getIntermediateState("arm64", "x86_64");
	if (oppositeState == nullptr)
	{
		Diagnostic::error("Universal Binary builder expects 'x86_64' or 'arm64' architecture.");
		return false;
	}

	if (!oppositeState->doBuild(Route::Build))
		return false;

	auto quiet = Output::quietNonBuild();
	Output::setQuietNonBuild(true);

	auto universalState = getIntermediateState("universal", "universal");
	if (universalState == nullptr)
	{
		Diagnostic::error("Universal Binary builder expects 'x86_64' or 'arm64' architecture.");
		return false;
	}

	Output::setQuietNonBuild(quiet);

	if (!createUniversalBinaries(m_state, *oppositeState, *universalState))
		return false;

	return bundleState(*universalState);
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

	auto buildState = std::make_unique<BuildState>(inputs);
	if (!buildState->initialize(m_installDependencies))
		return nullptr;

	return buildState;
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

	for (std::size_t i = 0; i < outputFilesA.size(); ++i)
	{
		auto& fileArchA = outputFilesA[i];
		auto& fileArchB = outputFilesB[i];
		auto& fileUniversal = outputFilesUniversal[i];

		// LOG(fileArchA, fileArchB, fileUniversal);

		// Example:
		// lipo -create -output universal-apple-darwin_Release/chalet x86_64-apple-darwin_Release/chalet  arm64-apple-darwin_Release/chalet

		if (!Commands::subprocess({ m_state.tools.lipo(), "-create", "-output", fileUniversal, std::move(fileArchA), std::move(fileArchB) }))
		{
			Diagnostic::error(fmt::format("There was an error making the binary: {}", fileUniversal));
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool UniversalBinaryMacOS::bundleState(BuildState& inState) const
{
	AppBundler bundler;
	const auto& buildFile = m_inputs.buildFile();
	for (auto& target : inState.distribution)
	{
		if (!bundler.run(target, inState, buildFile))
			return false;
	}

	return true;
}
}
