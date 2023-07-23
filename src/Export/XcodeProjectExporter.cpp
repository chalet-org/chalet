/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/XcodeProjectExporter.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/Xcode/XcodePBXProjGen.hpp"
#include "Export/Xcode/XcodeProjectSpecGen.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"

#define CHALET_XCODE_USE_SPEC_GEN 0

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

	if (!validateXcodeGenIsInstalled(typeName))
		return false;

	return true;
}

/*****************************************************************************/
bool XcodeProjectExporter::generateProjectFiles()
{
	if (!useProjectBuildDirectory(".xcode"))
		return false;

#if CHALET_XCODE_USE_SPEC_GEN
	XcodeProjectSpecGen specGen(m_states, m_directory);
	auto specFile = fmt::format("{}/project-spec.json", m_directory);
	if (!specGen.saveToFile(specFile))
	{
		Diagnostic::error("There was a problem saving the Xcode project spec file.");
		return false;
	}

	const auto& cwd = m_inputs.workingDirectory();

	StringList cmd{
		m_xcodegen,
		"--no-env",
		"--cache-path",
		fmt::format("{}/.xcodegen/cache/{{SPEC_PATH_HASH}}", m_directory),
		"--project",
		m_directory,
		"--project-root",
		cwd,
		"--spec",
		specFile,
		"--use-cache",
		"--quiet",
	};

	return Commands::subprocess(cmd);
#else
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

	XcodePBXProjGen slnGen(m_states);
	if (!slnGen.saveToFile(fmt::format("{}/project.pbxproj", xcodeproj)))
	{
		Diagnostic::error("There was a problem saving the project.pbxproj file.");
		return false;
	}

	LOG(xcodeproj);
	return false;
#endif
}

/*****************************************************************************/
bool XcodeProjectExporter::validateXcodeGenIsInstalled(const std::string& inTypeName)
{
	m_xcodegen = Commands::which("xcodegen");
	if (m_xcodegen.empty())
	{
		auto brew = Commands::which("brew");
		if (brew.empty())
		{
			Diagnostic::error("{} project format requires xcodegen, installed via Homebrew - https://brew.sh", inTypeName);
			return false;
		}
		else
		{
			Diagnostic::error("{} project format requires xcodegen, installed via 'brew install xcodegen'", inTypeName);
			return false;
		}
	}

	return true;
}
}
