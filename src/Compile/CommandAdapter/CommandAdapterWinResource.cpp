/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CommandAdapter/CommandAdapterWinResource.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "FileTemplates/PlatformFileTemplates.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CommandAdapterWinResource::CommandAdapterWinResource(const BuildState& inState, const SourceTarget& inProject) :
	m_state(inState),
	m_project(inProject)
{
}

/*****************************************************************************/
bool CommandAdapterWinResource::createWindowsApplicationManifest()
{
	if (m_project.isStaticLibrary())
		return true;

	auto& sources = m_state.cache.file().sources();

	const auto windowsManifestFile = m_state.paths.getWindowsManifestFilename(m_project);
	const auto windowsManifestResourceFile = m_state.paths.getWindowsManifestResourceFilename(m_project);
	if (windowsManifestFile.empty() || windowsManifestResourceFile.empty())
		return true;

	bool manifestChanged = sources.fileChangedOrDoesNotExist(windowsManifestFile);
	if (manifestChanged)
	{
		Files::removeIfExists(windowsManifestResourceFile);

		if (String::startsWith(m_state.paths.intermediateDir(m_project), windowsManifestFile) || !Files::pathExists(windowsManifestFile))
		{
			WindowsManifestGenSettings manifestSettings;
			manifestSettings.name = m_project.name();
			manifestSettings.cpu = m_state.info.targetArchitecture();
			manifestSettings.unicode = m_project.executionCharsetIsUnicode();
			manifestSettings.compatibility = true;

			if (m_project.hasMetadata())
				manifestSettings.version = m_project.metadata().version();

			if (!manifestSettings.version)
				manifestSettings.version = m_state.workspace.metadata().version();

			if (!manifestSettings.version)
				manifestSettings.version = Version::fromString("1.0.0.0");

			auto manifestContents = PlatformFileTemplates::windowsAppManifest(manifestSettings);
			if (!Files::createFileWithContents(windowsManifestFile, manifestContents))
			{
				Diagnostic::error("Error creating windows manifest file: {}", windowsManifestFile);
				return false;
			}
		}
	}

	if (manifestChanged || sources.fileChangedOrDoesNotExist(windowsManifestResourceFile))
	{
		auto rcContents = PlatformFileTemplates::windowsManifestResource(windowsManifestFile, m_project.isSharedLibrary());
		if (!Files::createFileWithContents(windowsManifestResourceFile, rcContents))
		{
			Diagnostic::error("Error creating windows manifest resource file: {}", windowsManifestResourceFile);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool CommandAdapterWinResource::createWindowsApplicationIcon()
{
	if (!m_project.isExecutable())
		return true;

	auto& sources = m_state.cache.file().sources();

	const auto& windowsIconFile = m_project.windowsApplicationIcon();
	const auto windowsIconResourceFile = m_state.paths.getWindowsIconResourceFilename(m_project);

	bool iconChanged = !windowsIconFile.empty() && sources.fileChangedOrDoesNotExist(windowsIconFile);
	if (iconChanged)
	{
		Files::removeIfExists(windowsIconResourceFile);

		if (!Files::pathExists(windowsIconFile))
		{
			Diagnostic::error("Windows icon does not exist: {}", windowsIconFile);
			return false;
		}
	}

	if (iconChanged || sources.fileChangedOrDoesNotExist(windowsIconResourceFile))
	{
		auto rcContents = PlatformFileTemplates::windowsIconResource(windowsIconFile);
		if (!Files::createFileWithContents(windowsIconResourceFile, rcContents))
		{
			Diagnostic::error("Error creating windows icon resource file: {}", windowsIconResourceFile);
			return false;
		}
	}

	return true;
}

}
