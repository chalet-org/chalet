/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Xcode/XcodeXSchemeGen.hpp"

#include "Bundler/AppBundlerMacOS.hpp"
#include "Bundler/BinaryDependency/BinaryDependencyMap.hpp"
#include "DotEnv/DotEnvFileGenerator.hpp"
#include "Export/TargetExportAdapter.hpp"
#include "Process/Environment.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Uuid.hpp"
#include "Xml/XmlFile.hpp"

namespace chalet
{
/*
<?xml version="1.0" encoding="UTF-8"?>
<Scheme
   LastUpgradeVersion = "1430"
   version = "1.7">
   <BuildAction
      parallelizeBuildables = "YES"
      buildImplicitDependencies = "YES">
   </BuildAction>
   <TestAction
      buildConfiguration = "Debug"
      selectedDebuggerIdentifier = "Xcode.DebuggerFoundation.Debugger.LLDB"
      selectedLauncherIdentifier = "Xcode.DebuggerFoundation.Launcher.LLDB"
      shouldUseLaunchSchemeArgsEnv = "YES"
      shouldAutocreateTestPlan = "YES">
   </TestAction>
   <LaunchAction
      buildConfiguration = "Debug"
      selectedDebuggerIdentifier = "Xcode.DebuggerFoundation.Debugger.LLDB"
      selectedLauncherIdentifier = "Xcode.DebuggerFoundation.Launcher.LLDB"
      launchStyle = "0"
      useCustomWorkingDirectory = "YES"
      customWorkingDirectory = "$(PROJECT_RUN_PATH)"
      ignoresPersistentStateOnLaunch = "NO"
      debugDocumentVersioning = "YES"
      debugServiceExtension = "internal"
      allowLocationSimulation = "YES"
      viewDebuggingEnabled = "No">
      <BuildableProductRunnable
         runnableDebuggingMode = "0">
         <BuildableReference
            BuildableIdentifier = "primary"
            BlueprintIdentifier = "F85F5EA79B42FAD8440084D6"
            BuildableName = "my-app"
            BlueprintName = "my-app"
            ReferencedContainer = "container:build/.xcode/project.xcodeproj">
         </BuildableReference>
      </BuildableProductRunnable>
   </LaunchAction>
   <ProfileAction
      buildConfiguration = "Profile"
      shouldUseLaunchSchemeArgsEnv = "YES"
      savedToolIdentifier = ""
      useCustomWorkingDirectory = "YES"
      customWorkingDirectory = "$(PROJECT_RUN_PATH)"
      debugDocumentVersioning = "YES">
      <BuildableProductRunnable
         runnableDebuggingMode = "0">
         <BuildableReference
            BuildableIdentifier = "primary"
            BlueprintIdentifier = "F85F5EA79B42FAD8440084D6"
            BuildableName = "my-app"
            BlueprintName = "my-app"
            ReferencedContainer = "container:build/.xcode/project.xcodeproj">
         </BuildableReference>
      </BuildableProductRunnable>
   </ProfileAction>
   <AnalyzeAction
      buildConfiguration = "Debug">
   </AnalyzeAction>
   <ArchiveAction
      buildConfiguration = "Release"
      revealArchiveInOrganizer = "YES">
   </ArchiveAction>
</Scheme>
*/
/*****************************************************************************/
XcodeXSchemeGen::XcodeXSchemeGen(std::vector<Unique<BuildState>>& inStates, const std::string& inXcodeProj, const std::string& inDebugConfig) :
	m_states(inStates),
	m_xcodeProj(inXcodeProj),
	m_debugConfiguration(inDebugConfig),
	m_xcodeNamespaceGuid("3C17F435-21B3-4D0A-A482-A276EDE1F0A2")
{
}

/*****************************************************************************/
bool XcodeXSchemeGen::createSchemes(const std::string& inSchemePath)
{
	StringList targetNames;

	std::string profileConfig;
	std::string releaseConfig{ "Release" };
	std::string otherRelease;
	std::unordered_map<std::string, std::string> configs;
	std::unordered_map<std::string, std::string> buildableNames;

	auto& firstState = *m_states.front();
	auto runArgumentMap = firstState.getCentralState().runArgumentMap();

	// Note: If a run argument has subtitution variables, we're only parsing them from this one state for now
	//   Maybe rework this later, but it's a relatively small use-case anyway - it really only affects build-specific variables
	//
	for (auto& target : firstState.targets)
	{
		auto& targetName = target->name();
		if (runArgumentMap.find(targetName) != runArgumentMap.end())
		{
			auto& arguments = runArgumentMap.at(targetName);
			for (auto& dir : arguments)
			{
				if (!firstState.replaceVariablesInString(dir, target.get()))
				{
					Diagnostic::error("There was an error parsing the run argument variables for: {}", target->name());
					return false;
				}
			}
		}
	}

	Dictionary<OrderedDictionary<std::string>> envMap;
	StringList noEnvs;

	bool foundRelease = false;
	for (auto& state : m_states)
	{
		auto& config = state->configuration;
		auto& configName = config.name();
		auto& buildDir = state->paths.buildOutputDir();
		auto& arch = state->info.targetArchitectureString();

		auto buildDirReplace = buildDir;
		String::replaceAll(buildDirReplace, configName, "$CONFIGURATION");
		String::replaceAll(buildDirReplace, arch, "$ARCHS");

		if (config.enableProfiling())
			profileConfig = configName;

		if (!config.debugSymbols() && !config.enableProfiling() && !config.enableSanitizers())
			otherRelease = configName;

		bool isRelease = String::equals(releaseConfig, configName);
		if (isRelease)
			foundRelease = true;

		auto env = DotEnvFileGenerator::make(*state);
		auto& environment = envMap[configName];

		auto path = env.getRunPaths();
		if (!path.empty())
		{
			String::replaceAll(path, buildDir, buildDirReplace);
			path = fmt::format("{}:${}", path, Environment::getPathKey());
			environment.emplace(Environment::getPathKey(), path);
		}

		auto libraryPath = env.getLibraryPath();
		if (!libraryPath.empty())
		{
			String::replaceAll(path, buildDir, buildDirReplace);
			libraryPath = fmt::format("{}:${}", libraryPath, Environment::getLibraryPathKey());
			environment.emplace(Environment::getLibraryPathKey(), libraryPath);
		}

		auto frameworkPath = env.getFrameworkPath();
		if (!frameworkPath.empty())
		{
			String::replaceAll(path, buildDir, buildDirReplace);
			frameworkPath = fmt::format("{}:${}", frameworkPath, Environment::getFrameworkPathKey());
			environment.emplace(Environment::getFrameworkPathKey(), frameworkPath);
		}

		StringList sourceTargets;
		for (auto& target : state->targets)
		{
			auto& name = target->name();
			if (List::addIfDoesNotExist(targetNames, name))
			{
				if (!String::equals(m_debugConfiguration, configName))
					configs[name] = configName;

				buildableNames[name] = name;
			}

			if (configs.find(name) != configs.end() && isRelease)
				configs[name] = configName;

			if (target->isSources())
				sourceTargets.emplace_back(target->name());
		}

#if defined(CHALET_MACOS)
		StringList sourceTargetsLowerCase;
		for (auto& target : sourceTargets)
			sourceTargetsLowerCase.emplace_back(String::toLowerCase(target));

		for (auto& target : state->distribution)
		{
			if (!target->isDistributionBundle())
				continue;

			auto& bundle = static_cast<BundleTarget&>(*target);
			if (bundle.isMacosAppBundle())
			{
				const auto& name = bundle.name();
				if (List::addIfDoesNotExist(targetNames, name))
				{
					if (!String::equals(m_debugConfiguration, configName))
						configs[name] = configName;

					switch (bundle.macosBundleType())
					{
						case MacOSBundleType::Application:
							buildableNames[name] = fmt::format("{}.app", name);
							break;
						// TODO: Other bundle types
						default:
							buildableNames[name] = name;
					}
				}

				if (configs.find(name) != configs.end() && isRelease)
					configs[name] = configName;

				if (runArgumentMap.find(name) != runArgumentMap.end())
					continue;

				BinaryDependencyMap dependencyMap(firstState);
				AppBundlerMacOS bundler(firstState, bundle, dependencyMap);
				if (bundler.initialize())
				{
					auto& mainExecutable = bundler.mainExecutable();
					if (!mainExecutable.empty())
					{
						if (runArgumentMap.find(mainExecutable) != runArgumentMap.end())
						{
							if (List::addIfDoesNotExist(targetNames, name))
							{
								runArgumentMap[name] = runArgumentMap.at(mainExecutable);
							}
						}
					}
				}

				List::addIfDoesNotExist(noEnvs, name);
			}
		}
#endif
	}

	if (!foundRelease && !otherRelease.empty())
		releaseConfig = otherRelease;

	// std::string arguments;
	// if (runArgumentMap.find(targetName) != runArgumentMap.end())
	// 	arguments = runArgumentMap.at(targetName);

	for (auto& target : targetNames)
	{
		auto targetHash = this->getTargetHash(target);
		auto filename = fmt::format("{}/{}.xcscheme", inSchemePath, buildableNames.at(target));
		XmlFile xmlFile(filename);

		auto& xmlRoot = xmlFile.getRoot();
		bool hasConfig = configs.find(target) != configs.end();
		auto& testConfig = m_debugConfiguration;
		auto& launchConfig = hasConfig ? configs.at(target) : m_debugConfiguration;
		auto& analyzeConfig = m_debugConfiguration;
		auto& archiveConfig = releaseConfig;

		xmlRoot.setName("Scheme");
		xmlRoot.addAttribute("LastUpgradeVersion", "1430");
		xmlRoot.addAttribute("version", "1.7");

		xmlRoot.addElement("BuildAction", [this](XmlElement& node) {
			node.addAttribute("parallelizeBuildables", getBoolString(true));
			node.addAttribute("buildImplicitDependencies", getBoolString(true));
		});
		xmlRoot.addElement("TestAction", [this, &testConfig](XmlElement& node) {
			node.addAttribute("buildConfiguration", testConfig);
			node.addAttribute("selectedDebuggerIdentifier", "Xcode.DebuggerFoundation.Debugger.LLDB");
			node.addAttribute("selectedLauncherIdentifier", "Xcode.DebuggerFoundation.Launcher.LLDB");
			node.addAttribute("shouldUseLaunchSchemeArgsEnv", getBoolString(true));
			node.addAttribute("shouldAutocreateTestPlan", getBoolString(true));
		});
		xmlRoot.addElement("LaunchAction", [this, &launchConfig, &runArgumentMap, &envMap, &target, &targetHash, &noEnvs, &buildableNames](XmlElement& node) {
			node.addAttribute("buildConfiguration", launchConfig);
			node.addAttribute("selectedDebuggerIdentifier", "Xcode.DebuggerFoundation.Debugger.LLDB");
			node.addAttribute("selectedLauncherIdentifier", "Xcode.DebuggerFoundation.Launcher.LLDB");
			node.addAttribute("launchStyle", "0");
			node.addAttribute("useCustomWorkingDirectory", getBoolString(true));
			node.addAttribute("customWorkingDirectory", this->getRunWorkingDirectoryForTargetWithCurrentDirectoryAs(target, "$(PROJECT_RUN_PATH)"));
			node.addAttribute("ignoresPersistentStateOnLaunch", getBoolString(false));
			node.addAttribute("debugDocumentVersioning", getBoolString(false));
			node.addAttribute("debugServiceExtension", "internal");
			node.addAttribute("allowLocationSimulation", getBoolString(true));
			node.addAttribute("viewDebuggingEnabled", "No");
			node.addElement("BuildableProductRunnable", [this, &target, &targetHash, &buildableNames](XmlElement& node2) {
				node2.addAttribute("runnableDebuggingMode", "0");
				node2.addElement("BuildableReference", [this, &target, &targetHash, &buildableNames](XmlElement& node3) {
					node3.addAttribute("BuildableIdentifier", "primary");
					node3.addAttribute("BlueprintIdentifier", targetHash);
					node3.addAttribute("BuildableName", buildableNames.at(target));
					node3.addAttribute("BlueprintName", target);
					node3.addAttribute("ReferencedContainer", fmt::format("container:{}", m_xcodeProj));
				});
			});

			if (runArgumentMap.find(target) != runArgumentMap.end())
			{
				auto& arguments = runArgumentMap.at(target);
				node.addElement("CommandLineArguments", [&arguments](XmlElement& node2) {
					for (auto& arg : arguments)
					{
						node2.addElement("CommandLineArgument", [&arg](XmlElement& node3) {
							node3.addAttribute("argument", arg);
							node3.addAttribute("isEnabled", "YES");
						});
					}
				});
			}

			if (!List::contains(noEnvs, target) && envMap.find(launchConfig) != envMap.end())
			{
				auto& environment = envMap.at(launchConfig);
				node.addElement("EnvironmentVariables", [this, &environment](XmlElement& node2) {
					for (const auto& it : environment)
					{
						auto& key = it.first;
						auto& value = it.second;

						node2.addElement("EnvironmentVariable", [this, &key, &value](XmlElement& node3) {
							node3.addAttribute("key", key);
							node3.addAttribute("value", value);
							node3.addAttribute("isEnabled", getBoolString(true));
						});
					}
				});
			}
		});
		xmlRoot.addElement("ProfileAction", [this, &profileConfig, &target, &targetHash, &buildableNames](XmlElement& node) {
			node.addAttribute("buildConfiguration", profileConfig);
			node.addAttribute("shouldUseLaunchSchemeArgsEnv", getBoolString(true));
			node.addAttribute("savedToolIdentifier", "");
			node.addAttribute("useCustomWorkingDirectory", getBoolString(true));
			node.addAttribute("customWorkingDirectory", "$(PROJECT_RUN_PATH)");
			node.addAttribute("debugDocumentVersioning", getBoolString(false));
			node.addElement("BuildableProductRunnable", [this, &target, &targetHash, &buildableNames](XmlElement& node2) {
				node2.addAttribute("runnableDebuggingMode", "0");
				node2.addElement("BuildableReference", [this, &target, &targetHash, &buildableNames](XmlElement& node3) {
					node3.addAttribute("BuildableIdentifier", "primary");
					node3.addAttribute("BlueprintIdentifier", targetHash);
					node3.addAttribute("BuildableName", buildableNames.at(target));
					node3.addAttribute("BlueprintName", target);
					node3.addAttribute("ReferencedContainer", fmt::format("container:{}", m_xcodeProj));
				});
			});
		});
		xmlRoot.addElement("AnalyzeAction", [&analyzeConfig](XmlElement& node) {
			node.addAttribute("buildConfiguration", analyzeConfig);
		});
		xmlRoot.addElement("ArchiveAction", [this, &archiveConfig](XmlElement& node) {
			node.addAttribute("buildConfiguration", archiveConfig);
			node.addAttribute("revealArchiveInOrganizer", getBoolString(true));
		});

		if (!xmlFile.save())
		{
			Diagnostic::error("There was a problem saving: {}", filename);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
std::string XcodeXSchemeGen::getTargetHash(const std::string& inTarget) const
{
	return Uuid::v5(fmt::format("{}_TARGET", inTarget), m_xcodeNamespaceGuid).toAppleHash();
}

/*****************************************************************************/
std::string XcodeXSchemeGen::getBoolString(const bool inValue) const
{
	return inValue ? "YES" : "NO";
}

/*****************************************************************************/
std::string XcodeXSchemeGen::getRunWorkingDirectoryForTargetWithCurrentDirectoryAs(const std::string& inTarget, const std::string& inCwdAlias) const
{
	for (auto& state : m_states)
	{
		if (!String::equals(m_debugConfiguration, state->configuration.name()))
			continue;

		for (auto& target : state->targets)
		{
			if (String::equals(inTarget, target->name()))
			{
				TargetExportAdapter adapter(*state, *target);
				return adapter.getRunWorkingDirectoryWithCurrentWorkingDirectoryAs(inCwdAlias);
			}
		}

		break;
	}

	return inCwdAlias;
}
}
