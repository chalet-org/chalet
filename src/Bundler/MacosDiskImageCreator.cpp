/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/MacosDiskImageCreator.hpp"

#include "Bundler/MacosCodeSignOptions.hpp"
#include "Bundler/MacosNotarizationMsg.hpp"
#include "Core/CommandLineInputs.hpp"
#include "FileTemplates/PlatformFileTemplates.hpp"
#include "Process/Process.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/MacosDiskImageTarget.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
MacosDiskImageCreator::MacosDiskImageCreator(const BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
bool MacosDiskImageCreator::make(const MacosDiskImageTarget& inDiskImage)
{
#if defined(CHALET_MACOS)
	m_diskName = String::getPathFolderBaseName(inDiskImage.name());

	const auto& distributionDirectory = m_state.inputs.distributionDirectory();

	auto& hdiutil = m_state.tools.hdiutil();
	auto& tiffutil = m_state.tools.tiffutil();
	const std::string volumePath = fmt::format("/Volumes/{}", m_diskName);

	Process::runNoOutput({ hdiutil, "detach", fmt::format("{}/", volumePath), "-force" });

	Timer timer;

	Diagnostic::stepInfoEllipsis("Creating the distribution disk image");

	const std::string tmpDmg = fmt::format("{}/.tmp.dmg", distributionDirectory);

	m_includedPaths.clear();
	for (auto& [path, _] : inDiskImage.positions())
	{
		if (String::equals("Applications", path))
			continue;

		for (auto& target : m_state.distribution)
		{
			if (target->isDistributionBundle() && String::equals(target->name(), path))
			{
				auto& bundle = static_cast<BundleTarget&>(*target);
				const auto& subdirectory = bundle.subdirectory();

				auto appPath = fmt::format("{}/{}.{}", subdirectory, path, bundle.macosBundleExtension());
				if (appPath.back() == '.')
					appPath.pop_back();

				if (!Files::pathExists(appPath))
				{
					Diagnostic::error("Path not found, but it's required by {}.dmg: {}", m_diskName, appPath);
					return false;
				}

				m_includedPaths.emplace(path, std::move(appPath));
				break;
			}
		}
	}

	uintmax_t appSize = 0;
	for (auto& [_, path] : m_includedPaths)
	{
		appSize += Files::getPathSize(path);
	}
	uintmax_t mb = 1000000;
	uintmax_t dmgSize = appSize > mb ? appSize / mb : 10;
	// if (dmgSize > 10)
	{
		uintmax_t temp = 16;
		while (temp < dmgSize)
		{
			temp += temp;
		}
		dmgSize = temp + 16;
	}

	if (!Process::runMinimalOutput({ hdiutil, "create", "-megabytes", std::to_string(dmgSize), "-fs", "HFS+", "-volname", m_diskName, tmpDmg }))
		return false;

	if (!Process::runMinimalOutput({ hdiutil, "attach", tmpDmg }))
		return false;

	const auto& background1x = inDiskImage.background1x();
	const auto& background2x = inDiskImage.background2x();
	const bool hasBackground = !background1x.empty();

	if (hasBackground)
	{
		const std::string backgroundPath = fmt::format("{}/.background", volumePath);
		if (!Files::makeDirectory(backgroundPath))
			return false;

		if (String::endsWith(".tiff", background1x))
		{
			if (!Files::copyRename(background1x, fmt::format("{}/background.tiff", backgroundPath)))
				return false;
		}
		else
		{
			StringList cmd{ tiffutil, "-cathidpicheck" };

			cmd.push_back(background1x);

			if (!background2x.empty())
				cmd.push_back(background2x);

			cmd.emplace_back("-out");
			cmd.emplace_back(fmt::format("{}/background.tiff", backgroundPath));

			if (!Process::runNoOutput(cmd))
				return false;
		}
	}

	if (inDiskImage.includeApplicationsSymlink())
	{
		if (!Files::createDirectorySymbolicLink("/Applications", fmt::format("{}/Applications", volumePath)))
			return false;
	}

	for (auto& [_, path] : m_includedPaths)
	{
		if (!Files::copySilent(path, volumePath))
			return false;
	}

	const auto applescriptText = getDmgApplescript(inDiskImage);

	if (!Process::run({ m_state.tools.osascript(), "-e", applescriptText }))
		return false;

	Files::removeRecursively(fmt::format("{}/.fseventsd", volumePath));

	if (!Process::runMinimalOutput({ hdiutil, "detach", fmt::format("{}/", volumePath) }))
		return false;

	std::string outDmgPath = fmt::format("{}/{}.dmg", distributionDirectory, m_diskName);
	if (!Process::runMinimalOutput({ hdiutil, "convert", tmpDmg, "-format", "UDZO", "-o", outDmgPath }))
		return false;

	if (!Files::removeRecursively(tmpDmg))
		return false;

	Diagnostic::printDone(timer.asString());

	return signDmgImage(outDmgPath);
#else
	UNUSED(inDiskImage);
	return false;
#endif
}

/*****************************************************************************/
bool MacosDiskImageCreator::signDmgImage(const std::string& inPath) const
{
	if (m_state.tools.signingIdentity().empty())
	{
		Diagnostic::warn("dmg '{}' was not signed - signingIdentity is not set, or was empty.", inPath);
		return true;
	}

	Timer timer;
	Diagnostic::stepInfoEllipsis("Signing the disk image");

	MacosCodeSignOptions entitlementOptions;
	if (!m_state.tools.macosCodeSignDiskImage(inPath, entitlementOptions))
	{
		Diagnostic::error("Failed to sign: {}", inPath);
		return false;
	}

	Diagnostic::printDone(timer.asString());

	MacosNotarizationMsg notarization(m_state);
	notarization.showMessage(inPath);

	return true;
}

/*****************************************************************************/
std::string MacosDiskImageCreator::getDmgApplescript(const MacosDiskImageTarget& inDiskImage) const
{
	std::string pathbar = inDiskImage.pathbarVisible() ? "true" : "false";

	u16 iconSize = inDiskImage.iconSize();
	u16 textSize = inDiskImage.textSize();

	u16 width = inDiskImage.size().width;
	u16 height = inDiskImage.size().height;

	u16 leftMost = std::numeric_limits<u16>::max();
	u16 bottomMost = height + (iconSize / static_cast<u16>(2)) + 16;

	std::string positions;
	for (auto& [path, pos] : inDiskImage.positions())
	{
		std::string label;
		if (String::equals("Applications", path))
		{
			label = path;
		}
		else
		{
			chalet_assert(m_includedPaths.find(path) != m_includedPaths.end(), "");

			const auto& outPath = m_includedPaths.at(path);
			label = String::getPathFilename(outPath);
		}

		if (pos.x > 0 && pos.x < leftMost)
		{
			leftMost = static_cast<u16>(pos.x);
		}

		positions += fmt::format(R"applescript(
  set position of item "{label}" of container window to {{{posX}, {posY}}})applescript",
			FMT_ARG(label),
			fmt::arg("posX", pos.x),
			fmt::arg("posY", pos.y));
	}

	std::string background;
	if (!inDiskImage.background1x().empty())
	{
		background = fmt::format(R"applescript(
  set background picture of viewOptions to file ".background:background.tiff"
  set position of item ".background" of container window to {{{leftMost}, {bottomMost}}})applescript",
			FMT_ARG(leftMost),
			FMT_ARG(bottomMost));
	}

	return fmt::format(R"applescript(tell application "Finder"
 tell disk "{diskName}"
  open
  set the bounds of container window to {{0, 0, {width}, {height}}}
  set viewOptions to the icon view options of container window
  set arrangement of viewOptions to not arranged
  set label position of viewOptions to bottom
  set text size of viewOptions to {textSize}
  set icon size of viewOptions to {iconSize}{positions}{background}
  set pathbar visible of container window to {pathbar}
  set toolbar visible of container window to false
  set statusbar visible of container window to false
  set current view of container window to icon view
  close
  update without registering applications
 end tell
end tell)applescript",
		fmt::arg("diskName", m_diskName),
		FMT_ARG(positions),
		FMT_ARG(background),
		FMT_ARG(pathbar),
		FMT_ARG(iconSize),
		FMT_ARG(textSize),
		FMT_ARG(width),
		FMT_ARG(height));
}

}
