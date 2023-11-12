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
#include "Terminal/Files.hpp"
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

	bool manifestChanged = sources.fileChangedOrDoesNotExist(windowsManifestFile);

	if (!windowsManifestFile.empty() && manifestChanged)
	{
		const bool isNative = m_state.toolchain.strategy() == StrategyType::Native;
		if (!isNative && Files::pathExists(windowsManifestResourceFile))
			Files::remove(windowsManifestResourceFile);

		if (!Files::pathExists(windowsManifestFile))
		{
			std::string manifestContents;
			if (m_project.windowsApplicationManifest().empty())
				manifestContents = PlatformFileTemplates::minimumWindowsAppManifest();
			else
				manifestContents = PlatformFileTemplates::generalWindowsAppManifest(m_project.name(), m_state.info.targetArchitecture());

			String::replaceAll(manifestContents, '\t', ' ');

			if (!Files::createFileWithContents(windowsManifestFile, manifestContents))
			{
				Diagnostic::error("Error creating windows manifest file: {}", windowsManifestFile);
				return false;
			}
		}
	}

	if (!windowsManifestResourceFile.empty() && (sources.fileChangedOrDoesNotExist(windowsManifestResourceFile) || manifestChanged))
	{
		std::string rcContents = PlatformFileTemplates::windowsManifestResource(windowsManifestFile, m_project.isSharedLibrary());
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

	if (!windowsIconFile.empty() && sources.fileChangedOrDoesNotExist(windowsIconFile))
	{
		const bool isNative = m_state.toolchain.strategy() == StrategyType::Native;
		if (!isNative && Files::pathExists(windowsIconResourceFile))
			Files::remove(windowsIconResourceFile);

		if (!Files::pathExists(windowsIconFile))
		{
			Diagnostic::error("Windows icon does not exist: {}", windowsIconFile);
			return false;
		}
	}

	if (!windowsIconResourceFile.empty() && sources.fileChangedOrDependantChanged(windowsIconResourceFile, windowsIconFile))
	{
		std::string rcContents = PlatformFileTemplates::windowsIconResource(windowsIconFile);
		if (!Files::createFileWithContents(windowsIconResourceFile, rcContents))
		{
			Diagnostic::error("Error creating windows icon resource file: {}", windowsIconResourceFile);
			return false;
		}
	}

	return true;
}

}
