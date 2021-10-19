/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Bundle/BundleMacOS.hpp"

#include "FileTemplates/PlatformFileTemplates.hpp"

#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
#if defined(CHALET_MACOS)
static struct
{
	const Dictionary<MacOSBundleType> bundleTypes{
		{ "app", MacOSBundleType::Application },
		{ "framework", MacOSBundleType::Framework },
		{ "plugin", MacOSBundleType::Plugin },
		{ "kext", MacOSBundleType::KernelExtension },
	};
} state;
#endif
}

/*****************************************************************************/
bool BundleMacOS::validate()
{
	bool result = true;

	if (m_bundleName.size() > 15)
	{
		Diagnostic::error("distribution.macos.bundleName should not contain more than 15 characters.");
		result = false;
	}

	if (!m_icon.empty())
	{
		if (!String::endsWith({ ".png", ".icns" }, m_icon))
		{
			Diagnostic::error("distribution.macos.icon must end with '.png' or '.icns', but was '{}'.", m_icon);
			result = false;
		}
		else if (!Commands::pathExists(m_icon))
		{
			Diagnostic::error("distribution.macos.icon '{}' was not found.", m_icon);
			result = false;
		}
	}

	if (!m_infoPropertyList.empty())
	{
		if (!String::endsWith(".json", m_infoPropertyList))
		{
			Diagnostic::error("distribution.macos.infoPropertyList must end with '.plist' or '.json', but was '{}'.", m_infoPropertyList);
			result = false;
		}
		else if (!Commands::pathExists(m_infoPropertyList))
		{
			std::ofstream(m_infoPropertyList) << PlatformFileTemplates::macosInfoPlist();
		}
	}

	if (!m_dmgBackground1x.empty())
	{
		if (!String::endsWith(".png", m_dmgBackground1x))
		{
			Diagnostic::error("distribution.macos.dmgBackground1x must end with '.png', but was '{}'.", m_dmgBackground1x);
			result = false;
		}
		else if (!Commands::pathExists(m_dmgBackground1x))
		{
			Diagnostic::error("distribution.macos.dmgBackground1x '{}' was not found.", m_dmgBackground1x);
			result = false;
		}
	}

	if (!m_dmgBackground2x.empty())
	{
		if (!String::endsWith(".png", m_dmgBackground2x))
		{
			Diagnostic::error("distribution.macos.dmgBackground2x must end with '.png', but was '{}'.", m_dmgBackground2x);
			result = false;
		}
		else if (!Commands::pathExists(m_dmgBackground2x))
		{
			Diagnostic::error("distribution.macos.dmgBackground2x '{}' was not found.", m_dmgBackground2x);
			result = false;
		}
	}

	return result;
}

/*****************************************************************************/
MacOSBundleType BundleMacOS::bundleType() const noexcept
{
	return m_bundleType;
}

void BundleMacOS::setBundleType(std::string&& inName)
{
	m_bundleType = getBundleTypeFromString(inName);
}

bool BundleMacOS::isAppBundle() const noexcept
{
	return m_bundleType == MacOSBundleType::Application;
}

/*****************************************************************************/
const std::string& BundleMacOS::bundleName() const noexcept
{
	return m_bundleName;
}

void BundleMacOS::setBundleName(const std::string& inValue)
{
	// bundleName is used specifically for CFBundleName
	// https://developer.apple.com/documentation/bundleresources/information_property_list/cfbundlename
	m_bundleName = inValue.substr(0, 15);
}

/*****************************************************************************/
const std::string& BundleMacOS::icon() const noexcept
{
	return m_icon;
}

void BundleMacOS::setIcon(std::string&& inValue)
{
	m_icon = std::move(inValue);
}

/*****************************************************************************/
const std::string& BundleMacOS::infoPropertyList() const noexcept
{
	return m_infoPropertyList;
}

void BundleMacOS::setInfoPropertyList(std::string&& inValue)
{
	if (String::endsWith(".plist", inValue))
	{
		m_infoPropertyList = inValue + ".json";
	}
	else
	{
		m_infoPropertyList = std::move(inValue);
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

void BundleMacOS::setDmgBackground1x(std::string&& inValue)
{
	m_dmgBackground1x = std::move(inValue);
}

/*****************************************************************************/
const std::string& BundleMacOS::dmgBackground2x() const noexcept
{
	return m_dmgBackground2x;
}

void BundleMacOS::setDmgBackground2x(std::string&& inValue)
{
	m_dmgBackground2x = std::move(inValue);
}

/*****************************************************************************/
MacOSBundleType BundleMacOS::getBundleTypeFromString(const std::string& inValue) const
{
#if defined(CHALET_MACOS)
	if (state.bundleTypes.find(inValue) != state.bundleTypes.end())
	{
		return state.bundleTypes.at(inValue);
	}
#else
	UNUSED(inValue);
#endif

	return MacOSBundleType::None;
}
}
