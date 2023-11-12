/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/MacosDiskImageTarget.hpp"

#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
MacosDiskImageTarget::MacosDiskImageTarget(const BuildState& inState) :
	IDistTarget(inState, DistTargetType::MacosDiskImage)
{
}

/*****************************************************************************/
bool MacosDiskImageTarget::initialize()
{
	if (!m_state.replaceVariablesInString(m_background1x, this))
		return false;

	if (!m_state.replaceVariablesInString(m_background2x, this))
		return false;

	return true;
}

/*****************************************************************************/
bool MacosDiskImageTarget::validate()
{
	bool result = true;

	if (!m_background1x.empty())
	{
		if (!String::endsWith(StringList{ ".png", ".tiff" }, m_background1x))
		{
			Diagnostic::error("macosDiskImage.background1x must end with '.png' or '.tiff', but was '{}'.", m_background1x);
			result = false;
		}
		else if (!Commands::pathExists(m_background1x))
		{
			Diagnostic::error("macosDiskImage.background1x '{}' was not found.", m_background1x);
			result = false;
		}
	}

	if (!m_background2x.empty())
	{
		if (!String::endsWith(".png", m_background2x))
		{
			Diagnostic::error("macosDiskImage.background2x must end with '.png', but was '{}'.", m_background2x);
			result = false;
		}
		else if (!Commands::pathExists(m_background2x))
		{
			Diagnostic::error("macosDiskImage.background2x '{}' was not found.", m_background2x);
			result = false;
		}
	}

	if (m_positions.empty())
	{
		Diagnostic::error("macosDiskImage.positions must contain at least one bundle or distribution path.", m_background2x);
		result = false;
	}

	auto diskName = String::getPathFolderBaseName(this->name());
	for (auto& [path, _] : m_positions)
	{
		bool foundPath = false;
		if (String::equals("Applications", path))
			continue;

		for (auto& target : m_state.distribution)
		{
			if (target->isDistributionBundle() && String::equals(target->name(), path))
				foundPath = true;
		}

		if (!foundPath)
		{
			Diagnostic::error("Bundle target required by {}.dmg was not found: {}", diskName, path);
			return false;
		}
	}

	return result;
}

/*****************************************************************************/
const std::string& MacosDiskImageTarget::background1x() const noexcept
{
	return m_background1x;
}

void MacosDiskImageTarget::setBackground1x(std::string&& inValue)
{
	m_background1x = std::move(inValue);
}

/*****************************************************************************/
const std::string& MacosDiskImageTarget::background2x() const noexcept
{
	return m_background2x;
}

void MacosDiskImageTarget::setBackground2x(std::string&& inValue)
{
	m_background2x = std::move(inValue);
}

/*****************************************************************************/
const Dictionary<Position<i16>>& MacosDiskImageTarget::positions() const noexcept
{
	return m_positions;
}
void MacosDiskImageTarget::addPosition(const std::string& inPath, const i16 inX, const i16 inY)
{
	auto path = String::getPathFolderBaseName(inPath);
	if (String::equals("Applications", path))
		m_includeApplicationsSymlink = true;

	m_positions[std::move(path)] = Position<i16>{ inX, inY };
}

/*****************************************************************************/
const Size<u16>& MacosDiskImageTarget::size() const noexcept
{
	return m_size;
}
void MacosDiskImageTarget::setSize(const u16 inWidth, const u16 inHeight) noexcept
{
	m_size.width = inWidth;
	m_size.height = inHeight;
}

/*****************************************************************************/
u16 MacosDiskImageTarget::iconSize() const noexcept
{
	return m_iconSize;
}
void MacosDiskImageTarget::setIconSize(const u16 inValue) noexcept
{
	m_iconSize = inValue;
}

/*****************************************************************************/
u16 MacosDiskImageTarget::textSize() const noexcept
{
	return m_textSize;
}
void MacosDiskImageTarget::setTextSize(const u16 inValue) noexcept
{
	m_textSize = inValue;
}

/*****************************************************************************/
bool MacosDiskImageTarget::pathbarVisible() const noexcept
{
	return m_pathbarVisible;
}
void MacosDiskImageTarget::setPathbarVisible(const bool inValue) noexcept
{
	m_pathbarVisible = inValue;
}

/*****************************************************************************/
bool MacosDiskImageTarget::includeApplicationsSymlink() const noexcept
{
	return m_includeApplicationsSymlink;
}

}
