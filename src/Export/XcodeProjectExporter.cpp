/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/XcodeProjectExporter.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/Xcode/XcodePBXProjGen.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"

namespace chalet
{
/*****************************************************************************/
XcodeProjectExporter::XcodeProjectExporter(const CommandLineInputs& inInputs) :
	IProjectExporter(inInputs, ExportKind::Xcode)
{
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
	if (!useProjectBuildDirectory(".xcode"))
		return false;

	auto xcodeproj = fmt::format("{}/project.xcodeproj", m_directory);
	if (!Commands::pathExists(xcodeproj))
		Commands::makeDirectory(xcodeproj);

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

	XcodePBXProjGen xcodeGen(m_states);
	if (!xcodeGen.saveToFile(fmt::format("{}/project.pbxproj", xcodeproj)))
	{
		Diagnostic::error("There was a problem saving the project.pbxproj file.");
		return false;
	}

	return true;
}
}
