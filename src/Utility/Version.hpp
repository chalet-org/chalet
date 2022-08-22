/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_UTIL_VERSION_HPP
#define CHALET_UTIL_VERSION_HPP

namespace chalet
{
struct Version
{
	static Version fromString(const std::string& inVersion);

	bool setFromString(const std::string& inVersion);

	bool hasMajor() const noexcept;
	uint major() const noexcept;

	bool hasMinor() const noexcept;
	uint minor() const noexcept;

	bool hasPatch() const noexcept;
	uint patch() const noexcept;

	bool hasTweak() const noexcept;
	uint tweak() const noexcept;

	std::string asString() const;

	bool operator<(const Version& rhs) const noexcept;

private:
	uint m_segments = 0;

	uint m_major = 0;
	uint m_minor = 0;
	uint m_patch = 0;
	uint m_tweak = 0;
};
}

#endif // CHALET_UTIL_VERSION_HPP
