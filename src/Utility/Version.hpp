/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct Version
{
	static Version fromString(const std::string& inVersion);

	operator bool() const noexcept;

	bool setFromString(const std::string& inVersion);

	bool hasMajor() const noexcept;
	u32 major() const noexcept;

	bool hasMinor() const noexcept;
	u32 minor() const noexcept;

	bool hasPatch() const noexcept;
	u32 patch() const noexcept;

	bool hasTweak() const noexcept;
	u32 tweak() const noexcept;

	std::string asString() const;
	std::string majorMinor() const;
	std::string majorMinorPatch() const;
	std::string majorMinorPatchTweak() const;

	bool operator<(const Version& rhs) const noexcept;

private:
	u32 m_segments = 0;

	u32 m_major = 0;
	u32 m_minor = 0;
	u32 m_patch = 0;
	u32 m_tweak = 0;
};
}
