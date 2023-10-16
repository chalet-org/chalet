/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Xcode/XcodeXSchemeGen.hpp"

#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
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
XcodeXSchemeGen::XcodeXSchemeGen(std::vector<Unique<BuildState>>& inStates, const std::string& inXcodeProj) :
	m_states(inStates),
	m_xcodeProj(inXcodeProj),
	m_xcodeNamespaceGuid("3C17F435-21B3-4D0A-A482-A276EDE1F0A2")
{
}

/*****************************************************************************/
bool XcodeXSchemeGen::createSchemes(const std::string& inSchemePath)
{
	StringList targetNames;
	for (auto& state : m_states)
	{
		for (auto& target : state->targets)
		{
			List::addIfDoesNotExist(targetNames, target->name());
		}
	}

	for (auto& target : targetNames)
	{
		auto filename = fmt::format("{}/{}.xcscheme", inSchemePath, target);
		XmlFile xmlFile(filename);

		auto& xmlRoot = xmlFile.getRoot();

		xmlRoot.setName("Scheme");
		xmlRoot.addAttribute("LastUpgradeVersion", "1430");
		xmlRoot.addAttribute("version", "1.7");

		xmlRoot.addElement("BuildAction", [this](XmlElement& node) {
			node.addAttribute("parallelizeBuildables", getBoolString(true));
			node.addAttribute("buildImplicitDependencies", getBoolString(true));
		});
		xmlRoot.addElement("TestAction", [this](XmlElement& node) {
			node.addAttribute("buildConfiguration", "Debug");
			node.addAttribute("selectedDebuggerIdentifier", "Xcode.DebuggerFoundation.Debugger.LLDB");
			node.addAttribute("selectedLauncherIdentifier", "Xcode.DebuggerFoundation.Launcher.LLDB");
			node.addAttribute("shouldUseLaunchSchemeArgsEnv", getBoolString(true));
			node.addAttribute("shouldAutocreateTestPlan", getBoolString(true));
		});
		xmlRoot.addElement("LaunchAction", [this, &target](XmlElement& node) {
			node.addAttribute("buildConfiguration", "Debug");
			node.addAttribute("selectedDebuggerIdentifier", "Xcode.DebuggerFoundation.Debugger.LLDB");
			node.addAttribute("selectedLauncherIdentifier", "Xcode.DebuggerFoundation.Launcher.LLDB");
			node.addAttribute("launchStyle", "0");
			node.addAttribute("useCustomWorkingDirectory", getBoolString(true));
			node.addAttribute("customWorkingDirectory", "$(PROJECT_RUN_PATH)");
			node.addAttribute("ignoresPersistentStateOnLaunch", getBoolString(false));
			node.addAttribute("debugDocumentVersioning", getBoolString(true));
			node.addAttribute("debugServiceExtension", "internal");
			node.addAttribute("allowLocationSimulation", getBoolString(true));
			node.addAttribute("viewDebuggingEnabled", "No");
			node.addElement("BuildableProductRunnable", [this, &target](XmlElement& node2) {
				node2.addAttribute("runnableDebuggingMode", "0");
				node2.addElement("BuildableReference", [this, &target](XmlElement& node3) {
					node3.addAttribute("BuildableIdentifier", "primary");
					node3.addAttribute("BlueprintIdentifier", this->getTargetHash(target));
					node3.addAttribute("BuildableName", target);
					node3.addAttribute("BlueprintName", target);
					node3.addAttribute("ReferencedContainer", fmt::format("container:{}", m_xcodeProj));
				});
			});
		});
		xmlRoot.addElement("ProfileAction", [this, &target](XmlElement& node) {
			node.addAttribute("buildConfiguration", "Release");
			node.addAttribute("shouldUseLaunchSchemeArgsEnv", getBoolString(true));
			node.addAttribute("savedToolIdentifier", "");
			node.addAttribute("useCustomWorkingDirectory", getBoolString(true));
			node.addAttribute("customWorkingDirectory", "$(PROJECT_RUN_PATH)");
			node.addAttribute("debugDocumentVersioning", getBoolString(true));
			node.addElement("BuildableProductRunnable", [this, &target](XmlElement& node2) {
				node2.addAttribute("runnableDebuggingMode", "0");
				node2.addElement("BuildableReference", [this, &target](XmlElement& node3) {
					node3.addAttribute("BuildableIdentifier", "primary");
					node3.addAttribute("BlueprintIdentifier", this->getTargetHash(target));
					node3.addAttribute("BuildableName", target);
					node3.addAttribute("BlueprintName", target);
					node3.addAttribute("ReferencedContainer", fmt::format("container:{}", m_xcodeProj));
				});
			});
		});
		xmlRoot.addElement("AnalyzeAction", [](XmlElement& node) {
			node.addAttribute("buildConfiguration", "Debug");
		});
		xmlRoot.addElement("ArchiveAction", [this](XmlElement& node) {
			node.addAttribute("buildConfiguration", "Release");
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

}
