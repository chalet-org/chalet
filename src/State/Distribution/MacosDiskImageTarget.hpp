/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MACOS_DISK_IMAGE_TARGET_HPP
#define CHALET_MACOS_DISK_IMAGE_TARGET_HPP

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

	const Dictionary<Position<short>>& positions() const noexcept;
	void addPosition(const std::string& inPath, const short inX, const short inY);

	const Size<ushort>& size() const noexcept;
	void setSize(const ushort inWidth, const ushort inHeight) noexcept;

	ushort iconSize() const noexcept;
	void setIconSize(const ushort inValue) noexcept;

	ushort textSize() const noexcept;
	void setTextSize(const ushort inValue) noexcept;

	bool pathbarVisible() const noexcept;
	void setPathbarVisible(const bool inValue) noexcept;

	bool includeApplicationsSymlink() const noexcept;

private:
	std::string m_background1x;
	std::string m_background2x;

	Dictionary<Position<short>> m_positions;

	Size<ushort> m_size{ 512, 342 };

	ushort m_iconSize = 48;
	ushort m_textSize = 12;

	bool m_pathbarVisible = false;
	bool m_includeApplicationsSymlink = false;
};
}

#endif // CHALET_MACOS_DISK_IMAGE_TARGET_HPP
