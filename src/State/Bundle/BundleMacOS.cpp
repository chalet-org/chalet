/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Bundle/BundleMacOS.hpp"

#include "Builder/PlatformFile.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool BundleMacOS::validate()
{
	bool result = true;
	{
		if (m_bundleName.size() > 15)
		{
			Diagnostic::error("bundle.macos.bundleName should not contain more than 15 characters.");
			result = false;
		}
	}

	if (!m_icon.empty())
	{
		if (!String::endsWith({ ".png", ".icns" }, m_icon))
		{
			Diagnostic::error(fmt::format("bundle.macos.icon must end with '.png' or '.icns', but was '{}'.", m_icon));
			result = false;
		}
		else if (!Commands::pathExists(m_icon))
		{
			Diagnostic::error("bundle.macos.icon was not found.");
			result = false;
		}
	}

	if (!m_infoPropertyList.empty())
	{
		if (!String::endsWith(".json", m_infoPropertyList))
		{
			Diagnostic::error(fmt::format("bundle.macos.infoPropertyList must end with '.plist' or '.json', but was '{}'.", m_infoPropertyList));
			result = false;
		}
		else if (!Commands::pathExists(m_infoPropertyList))
		{
			std::ofstream(m_infoPropertyList) << PlatformFile::macosInfoPlist();
		}
	}

	if (!m_dmgBackground1x.empty())
	{
		if (!String::endsWith(".png", m_dmgBackground1x))
		{
			Diagnostic::error(fmt::format("bundle.macos.dmgBackground1x must end with '.png', but was '{}'.", m_dmgBackground1x));
			result = false;
		}
		else if (!Commands::pathExists(m_dmgBackground1x))
		{
			Diagnostic::error("bundle.macos.dmgBackground1x was not found.");
			result = false;
		}
	}

	if (!m_dmgBackground2x.empty())
	{
		if (!String::endsWith(".png", m_dmgBackground2x))
		{
			Diagnostic::error(fmt::format("bundle.macos.dmgBackground2x must end with '.png', but was '{}'.", m_dmgBackground2x));
			result = false;
		}
		else if (!Commands::pathExists(m_dmgBackground2x))
		{
			Diagnostic::error("bundle.macos.dmgBackground2x was not found.");
			result = false;
		}
	}

	return result;
}

/*****************************************************************************/
const std::string& BundleMacOS::bundleName() const noexcept
{
	return m_bundleName;
}

void BundleMacOS::setBundleName(const std::string& inValue)
{
	m_bundleName = inValue;
}

/*****************************************************************************/
const std::string& BundleMacOS::icon() const noexcept
{
	return m_icon;
}

void BundleMacOS::setIcon(const std::string& inValue)
{
	m_icon = inValue;
}

/*****************************************************************************/
const std::string& BundleMacOS::infoPropertyList() const noexcept
{
	return m_infoPropertyList;
}

void BundleMacOS::setInfoPropertyList(const std::string& inValue)
{
	if (String::endsWith(".plist", inValue))
	{
		m_infoPropertyList = inValue + ".json";
	}
	else
	{
		m_infoPropertyList = inValue;
	}
}

/*****************************************************************************/
const std::string& BundleMacOS::infoPropertyListContent() const noexcept
{
	return m_infoPropertyListContent;
}

void BundleMacOS::setInfoPropertyListContent(std::string&& inValue)
{
	m_infoPropertyListContent = std::move(inValue);
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
	m_dmgBackground1x = inValue;
}

/*****************************************************************************/
const std::string& BundleMacOS::dmgBackground2x() const noexcept
{
	return m_dmgBackground2x;
}

void BundleMacOS::setDmgBackground2x(const std::string& inValue)
{
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
