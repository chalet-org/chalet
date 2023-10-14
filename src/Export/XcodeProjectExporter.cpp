/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/XcodeProjectExporter.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/Xcode/XcodePBXProjGen.hpp"
#include "Export/Xcode/XcodeXSchemeGen.hpp"
#include "State/BuildState.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"

namespace chalet
{
/*****************************************************************************/
XcodeProjectExporter::XcodeProjectExporter(const CommandLineInputs& inInputs) :
	IProjectExporter(inInputs, ExportKind::Xcode)
{
}

/*****************************************************************************/
std::string XcodeProjectExporter::getMainProjectOutput(const BuildState& inState)
{
	if (m_directory.empty())
	{
		if (!useProjectBuildDirectory(".xcode"))
			return std::string();
	}

	auto project = getProjectName(inState);
	return fmt::format("{}/{}.xcodeproj", m_directory, project);
}

/*****************************************************************************/
std::string XcodeProjectExporter::getMainProjectOutput()
{
	chalet_assert(!m_states.empty(), "states were empty getting project name");
	return getMainProjectOutput(*m_states.front());
}

/*****************************************************************************/
std::string XcodeProjectExporter::getProjectTypeName() const
{
	return std::string("Xcode");
}

/*****************************************************************************/
bool XcodeProjectExporter::validate(const BuildState& inState)
{
	auto typeName = getProjectTypeName();
	if (!inState.environment->isAppleClang())
	{
		// TODO: also check to make sure the Xcode path is set via xcode-select
		//
#if defined(CHALET_MACOS)
		Diagnostic::error("{} project format requires Xcode (Apple clang toolchain) (set with --toolchain/-t).", typeName);
#else
		Diagnostic::error("{} project format requires Xcode (Apple clang toolchain) on macOS.", typeName);
#endif
		return false;
	}

	return true;
}

/*****************************************************************************/
bool XcodeProjectExporter::generateProjectFiles()
{
	auto xcodeproj = getMainProjectOutput();
	if (xcodeproj.empty())
		return false;

	if (!Commands::pathExists(xcodeproj))
		Commands::makeDirectory(xcodeproj);

	auto project = getProjectName(*m_states.front());

	auto xcworkspace = fmt::format("{}/project.xcworkspace", xcodeproj);
	if (!Commands::pathExists(xcworkspace))
		Commands::makeDirectory(xcworkspace);

	// contents.xcworkspacedata
	auto xcworkspacedata = fmt::format("{}/contents.xcworkspacedata", xcworkspace);
	if (!Commands::pathExists(xcworkspacedata))
	{
		Commands::createFileWithContents(xcworkspacedata, R"xml(<?xml version="1.0" encoding="UTF-8"?>
<Workspace version="1.0">
   <FileRef location="self:">
   </FileRef>
</Workspace>)xml");
	}

	auto xcschemes = fmt::format("{}/xcshareddata/xcschemes", xcodeproj);
	if (!Commands::pathExists(xcschemes))
		Commands::makeDirectory(xcschemes);

	{
		XcodeXSchemeGen schemaGen(m_states, xcodeproj);
		if (!schemaGen.createSchemes(xcschemes))
		{
			Diagnostic::error("There was a problem creating the xcschemes files.");
			return false;
		}
	}

	{
		auto allBuildTargetName = getAllBuildTargetName();
		XcodePBXProjGen xcodeGen(m_states, allBuildTargetName);
		if (!xcodeGen.saveToFile(fmt::format("{}/project.pbxproj", xcodeproj)))
		{
			Diagnostic::error("There was a problem saving the project.pbxproj file.");
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
std::string XcodeProjectExporter::getProjectName(const BuildState& inState) const
{
	const auto& workspaceName = inState.workspace.metadata().name();
	return !workspaceName.empty() ? workspaceName : std::string("project");
}

}
