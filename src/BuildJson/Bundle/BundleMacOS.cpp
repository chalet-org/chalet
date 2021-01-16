/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/Bundle/BundleMacOS.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

#include "Builder/PlatformFile.hpp"

namespace chalet
{
/*****************************************************************************/
const std::string& BundleMacOS::bundleName() const noexcept
{
	return m_bundleName;
}

void BundleMacOS::setBundleName(const std::string& inValue)
{
	if (inValue.size() > 15)
	{
		Diagnostic::warn(fmt::format("{} (bundle.macos.bundleName) should not contain more than 15 characters. The value will be trimmed.", inValue));
		m_bundleName = inValue.substr(0, 15);
		return;
	}

	m_bundleName = inValue;
}

/*****************************************************************************/
const std::string& BundleMacOS::bundleIdentifier() const noexcept
{
	return m_bundleIdentifier;
}

void BundleMacOS::setBundleIdentifier(const std::string& inValue)
{
	m_bundleIdentifier = inValue;
}

/*****************************************************************************/
const std::string& BundleMacOS::icon() const noexcept
{
	return m_icon;
}

void BundleMacOS::setIcon(const std::string& inValue)
{
	fs::path path{ inValue };

	if (!path.has_extension() || (path.extension() != ".png" && path.extension() != ".icns"))
	{
		Diagnostic::errorAbort(fmt::format("{} (bundle.macos.icon) must be '.png' or '.icns'. Aborting...", inValue));
		return;
	}

	if (!Commands::pathExists(path))
	{
		Diagnostic::errorAbort(fmt::format("{} (bundle.macos.icon) was not found. Aborting...", inValue));
		return;
	}

	m_icon = inValue;
}

/*****************************************************************************/
const std::string& BundleMacOS::infoPropertyList() const noexcept
{
	return m_infoPropertyList;
}
void BundleMacOS::setInfoPropertyList(const std::string& inValue)
{
	fs::path path{ inValue };
	const std::string ext = path.extension().string();

	// TODO: validate this better
	const bool isPlist = String::equals(ext, ".plist");
	const bool isJson = String::equals(ext, ".json");

	if (ext.empty() || (!isPlist && !isJson))
	{
		Diagnostic::errorAbort(fmt::format("{} (bundle.macos.infoPropertyList) must be '.plist' or '.json'. Aborting...", inValue));
		return;
	}

	m_infoPropertyList = inValue;

	if (isPlist)
	{
		m_infoPropertyList += ".json";
		path /= ".json";
	}

	if (!Commands::pathExists(path))
	{
		std::ofstream(path) << PlatformFile::macosInfoPlist();
	}

	m_infoPropertyList = inValue;
}

/*****************************************************************************/
bool BundleMacOS::makeDmg() const noexcept
{
	return m_makeDmg;
}

void BundleMacOS::setMakeDmg(const bool inValue) noexcept
{
	m_makeDmg = inValue;
}

/*****************************************************************************/
const std::string& BundleMacOS::dmgBackground1x() const noexcept
{
	return m_dmgBackground1x;
}

void BundleMacOS::setDmgBackground1x(const std::string& inValue)
{
	fs::path path{ inValue };

	if (!path.has_extension() || path.extension() != ".png")
	{
		Diagnostic::errorAbort(fmt::format("{} (bundle.macos.dmgBackground1x) must be '.png'. Aborting...", inValue));
		return;
	}

	if (!Commands::pathExists(path))
	{
		Diagnostic::errorAbort(fmt::format("{} (bundle.macos.dmgBackground1x) was not found. Aborting...", inValue));
		return;
	}

	m_dmgBackground1x = inValue;
}

/*****************************************************************************/
const std::string& BundleMacOS::dmgBackground2x() const noexcept
{
	return m_dmgBackground2x;
}

void BundleMacOS::setDmgBackground2x(const std::string& inValue)
{
	fs::path path{ inValue };

	if (!path.has_extension() || path.extension() != ".png")
	{
		Diagnostic::errorAbort(fmt::format("{} (bundle.macos.dmgBackground2x) must be '.png'. Aborting...", inValue));
		return;
	}

	if (!Commands::pathExists(path))
	{
		Diagnostic::errorAbort(fmt::format("{} (bundle.macos.dmgBackground2x) was not found. Aborting...", inValue));
		return;
	}

	m_dmgBackground2x = inValue;
}

/*****************************************************************************/
const StringList& BundleMacOS::dylibs() const noexcept
{
	return m_dylibs;
}

void BundleMacOS::addDylibs(StringList& inList)
{
	List::forEach(inList, this, &BundleMacOS::addDylib);
}

void BundleMacOS::addDylib(std::string& inValue)
{
	List::addIfDoesNotExist(m_dylibs, std::move(inValue));
}

}
