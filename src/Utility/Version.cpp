/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Version.hpp"

#include "Utility/String.hpp"

namespace chalet
{
Version::operator bool() const noexcept
{
	return m_segments > 0;
}

/*****************************************************************************/
Version Version::fromString(const std::string& inVersion)
{
	Version ret;
	ret.setFromString(inVersion);
	return ret;
}

/*****************************************************************************/
bool Version::setFromString(const std::string& inVersion)
{
	m_segments = 0;

	if (inVersion.empty() || inVersion.find_first_not_of("1234567890.") != std::string::npos)
		return false;

	auto split = String::split(inVersion, '.');
	if (split.empty())
		return false;

	m_segments = static_cast<u32>(split.size());

	m_major = static_cast<u32>(atoi(split[0].c_str()));

	if (split.size() > 1)
	{
		m_minor = static_cast<u32>(atoi(split[1].c_str()));

		if (split.size() > 2)
		{
			m_patch = static_cast<u32>(atoi(split[2].c_str()));

			if (split.size() > 3)
			{
				m_tweak = static_cast<u32>(atoi(split[3].c_str()));
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool Version::hasMajor() const noexcept
{
	return m_segments > 0;
}
u32 Version::major() const noexcept
{
	return m_major;
}

/*****************************************************************************/
bool Version::hasMinor() const noexcept
{
	return m_segments > 1;
}
u32 Version::minor() const noexcept
{
	return m_minor;
}

/*****************************************************************************/
bool Version::hasPatch() const noexcept
{
	return m_segments > 2;
}
u32 Version::patch() const noexcept
{
	return m_patch;
}

/*****************************************************************************/
bool Version::hasTweak() const noexcept
{
	return m_segments > 3;
}
u32 Version::tweak() const noexcept
{
	return m_tweak;
}

/*****************************************************************************/
std::string Version::asString() const
{
	if (!hasMajor())
		return std::string();

	if (!hasMinor())
		return fmt::format("{}", m_major);

	if (!hasPatch())
		return fmt::format("{}.{}", m_major, m_minor);

	if (!hasTweak())
		return fmt::format("{}.{}.{}", m_major, m_minor, m_patch);

	return fmt::format("{}.{}.{}.{}", m_major, m_minor, m_patch, m_tweak);
}

/*****************************************************************************/
std::string Version::majorMinor() const
{
	return fmt::format("{}.{}", m_major, m_minor);
}

/*****************************************************************************/
std::string Version::majorMinorPatch() const
{
	return fmt::format("{}.{}.{}", m_major, m_minor, m_patch);
}

/*****************************************************************************/
std::string Version::majorMinorPatchTweak() const
{
	return fmt::format("{}.{}.{}.{}", m_major, m_minor, m_patch, m_tweak);
}

/*****************************************************************************/
bool Version::operator<(const Version& rhs) const noexcept
{
	bool res = false;

	if (hasMajor() && rhs.hasMajor())
		res |= m_major < rhs.m_major;

	if (hasMinor() && rhs.hasMinor())
		res |= (m_major == rhs.m_major && m_minor < rhs.m_minor);

	if (hasPatch() && rhs.hasPatch())
		res |= (m_major == rhs.m_major && m_minor == rhs.m_minor && m_patch < rhs.m_patch);

	if (hasTweak() && rhs.hasTweak())
		res |= (m_major == rhs.m_major && m_minor == rhs.m_minor && m_patch == rhs.m_patch && m_tweak < rhs.m_tweak);

	return res;
}
}
