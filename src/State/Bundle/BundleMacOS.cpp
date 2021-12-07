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
Dictionary<MacOSBundleType> getBundleTypes()
{
	return {
		{ "app", MacOSBundleType::Application },
		{ "framework", MacOSBundleType::Framework },
		{ "plugin", MacOSBundleType::Plugin },
		{ "kext", MacOSBundleType::KernelExtension },
	};
}
}

/*****************************************************************************/
bool BundleMacOS::validate()
{
	bool result = true;

	if (m_bundleName.size() > 15)
	{
		Diagnostic::error("bundle.macos.bundleName should not contain more than 15 characters.");
		result = false;
	}

	if (!m_icon.empty())
	{
		if (!String::endsWith({ ".png", ".icns" }, m_icon))
		{
			Diagnostic::error("bundle.macos.icon must end with '.png' or '.icns', but was '{}'.", m_icon);
			result = false;
		}
		else if (!Commands::pathExists(m_icon))
		{
			Diagnostic::error("bundle.macos.icon '{}' was not found.", m_icon);
			result = false;
		}
	}

	if (!m_infoPropertyList.empty())
	{
		if (!String::endsWith({ ".plist", ".json" }, m_infoPropertyList))
		{
			Diagnostic::error("bundle.macos.infoPropertyList must end with '.plist' or '.json', but was '{}'.", m_infoPropertyList);
			result = false;
		}
		else if (!Commands::pathExists(m_infoPropertyList))
		{
			if (String::endsWith(".plist", m_infoPropertyList))
			{
				Diagnostic::error("bundle.macos.infoPropertyList '{}' was not found.", m_infoPropertyList);
				result = false;
			}
			else
			{
				std::ofstream(m_infoPropertyList) << PlatformFileTemplates::macosInfoPlist();
			}
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

	if (m_bundleType != MacOSBundleType::None)
		m_bundleExtension = std::move(inName);
}

bool BundleMacOS::isAppBundle() const noexcept
{
	return m_bundleType == MacOSBundleType::Application;
}

/*****************************************************************************/
const std::string& BundleMacOS::bundleExtension() const noexcept
{
	return m_bundleExtension;
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
	m_infoPropertyList = std::move(inValue);
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
MacOSBundleType BundleMacOS::getBundleTypeFromString(const std::string& inValue) const
{
	auto bundleTypes = getBundleTypes();
	if (bundleTypes.find(inValue) != bundleTypes.end())
	{
		return bundleTypes.at(inValue);
	}

	return MacOSBundleType::None;
}
}
