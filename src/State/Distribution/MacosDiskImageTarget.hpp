/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/Distribution/IDistTarget.hpp"
#include "Utility/Position.hpp"
#include "Utility/Size.hpp"

namespace chalet
{
struct MacosDiskImageTarget final : public IDistTarget
{
	explicit MacosDiskImageTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;

	const std::string& background1x() const noexcept;
	void setBackground1x(std::string&& inValue);

	const std::string& background2x() const noexcept;
	void setBackground2x(std::string&& inValue);

	const Dictionary<Position<i16>>& positions() const noexcept;
	void addPosition(const std::string& inPath, const i16 inX, const i16 inY);

	const Size<u16>& size() const noexcept;
	void setSize(const u16 inWidth, const u16 inHeight) noexcept;

	u16 iconSize() const noexcept;
	void setIconSize(const u16 inValue) noexcept;

	u16 textSize() const noexcept;
	void setTextSize(const u16 inValue) noexcept;

	bool pathbarVisible() const noexcept;
	void setPathbarVisible(const bool inValue) noexcept;

	bool includeApplicationsSymlink() const noexcept;

private:
	std::string m_background1x;
	std::string m_background2x;

	Dictionary<Position<i16>> m_positions;

	Size<u16> m_size{ 512, 342 };

	u16 m_iconSize = 48;
	u16 m_textSize = 12;

	bool m_pathbarVisible = false;
	bool m_includeApplicationsSymlink = false;
};
}
