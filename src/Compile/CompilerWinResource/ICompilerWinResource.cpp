/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerWinResource/ICompilerWinResource.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "FileTemplates/PlatformFileTemplates.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

#include "Compile/CompilerWinResource/CompilerWinResourceGNUWindRes.hpp"
#include "Compile/CompilerWinResource/CompilerWinResourceLLVMRC.hpp"
#include "Compile/CompilerWinResource/CompilerWinResourceVisualStudioRC.hpp"

namespace chalet
{
/*****************************************************************************/
ICompilerWinResource::ICompilerWinResource(const BuildState& inState, const SourceTarget& inProject) :
	IToolchainExecutableBase(inState, inProject)
{
}

/*****************************************************************************/
[[nodiscard]] Unique<ICompilerWinResource> ICompilerWinResource::make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject)
{
	UNUSED(inType);

	const auto executable = String::toLowerCase(String::getPathFolderBaseName(String::getPathFilename(inExecutable)));
	// LOG("ICompilerWinResource:", static_cast<int>(inType), executable);

	if (String::equals("rc", executable))
		return std::make_unique<CompilerWinResourceVisualStudioRC>(inState, inProject);
	else if (String::equals("llvm-rc", executable))
		return std::make_unique<CompilerWinResourceLLVMRC>(inState, inProject);

	return std::make_unique<CompilerWinResourceGNUWindRes>(inState, inProject);
}

/*****************************************************************************/
bool ICompilerWinResource::initialize()
{
	if (!createWindowsApplicationManifest())
		return false;

	if (!createWindowsApplicationIcon())
		return false;

	return true;
}

/*****************************************************************************/
void ICompilerWinResource::addIncludes(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerWinResource::addDefines(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
bool ICompilerWinResource::createWindowsApplicationManifest()
{
	if (m_project.isStaticLibrary())
		return true;

	auto& sources = m_state.cache.file().sources();

	const auto windowsManifestFile = m_state.paths.getWindowsManifestFilename(m_project);
	const auto windowsManifestResourceFile = m_state.paths.getWindowsManifestResourceFilename(m_project);

	bool manifestChanged = sources.fileChangedOrDoesNotExist(windowsManifestFile);

	if (!windowsManifestFile.empty() && manifestChanged)
	{
		const bool isNative = m_state.toolchain.strategy() == StrategyType::Native;
		if (!isNative && Commands::pathExists(windowsManifestResourceFile))
			Commands::remove(windowsManifestResourceFile);

		if (!Commands::pathExists(windowsManifestFile))
		{
			std::string manifestContents;
			if (m_project.windowsApplicationManifest().empty())
				manifestContents = PlatformFileTemplates::minimumWindowsAppManifest();
			else
				manifestContents = PlatformFileTemplates::generalWindowsAppManifest(m_project.name(), m_state.info.targetArchitecture());

			String::replaceAll(manifestContents, '\t', ' ');

			if (!Commands::createFileWithContents(windowsManifestFile, manifestContents))
			{
				Diagnostic::error("Error creating windows manifest file: {}", windowsManifestFile);
				return false;
			}
		}
	}

	if (!windowsManifestResourceFile.empty() && (sources.fileChangedOrDoesNotExist(windowsManifestResourceFile) || manifestChanged))
	{
		std::string rcContents = PlatformFileTemplates::windowsManifestResource(windowsManifestFile, m_project.isSharedLibrary());
		if (!Commands::createFileWithContents(windowsManifestResourceFile, rcContents))
		{
			Diagnostic::error("Error creating windows manifest resource file: {}", windowsManifestResourceFile);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool ICompilerWinResource::createWindowsApplicationIcon()
{
	if (!m_project.isExecutable())
		return true;

	auto& sources = m_state.cache.file().sources();

	const auto& windowsIconFile = m_project.windowsApplicationIcon();
	const auto windowsIconResourceFile = m_state.paths.getWindowsIconResourceFilename(m_project);

	if (!windowsIconFile.empty() && sources.fileChangedOrDoesNotExist(windowsIconFile))
	{
		const bool isNative = m_state.toolchain.strategy() == StrategyType::Native;
		if (!isNative && Commands::pathExists(windowsIconResourceFile))
			Commands::remove(windowsIconResourceFile);

		if (!Commands::pathExists(windowsIconFile))
		{
			Diagnostic::error("Windows icon does not exist: {}", windowsIconFile);
			return false;
		}
	}

	if (!windowsIconResourceFile.empty() && sources.fileChangedOrDependantChanged(windowsIconResourceFile, windowsIconFile))
	{
		std::string rcContents = PlatformFileTemplates::windowsIconResource(windowsIconFile);
		if (!Commands::createFileWithContents(windowsIconResourceFile, rcContents))
		{
			Diagnostic::error("Error creating windows icon resource file: {}", windowsIconResourceFile);
			return false;
		}
	}

	return true;
}

}
