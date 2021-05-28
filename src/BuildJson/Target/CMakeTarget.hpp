/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CMAKE_TARGET_HPP
#define CHALET_CMAKE_TARGET_HPP

#include "BuildJson/Target/IBuildTarget.hpp"

namespace chalet
{
struct CMakeTarget final : public IBuildTarget
{
	explicit CMakeTarget(const BuildState& inState);

	const std::string& location() const noexcept;
	void setLocation(std::string&& inValue) noexcept;

	const StringList& defines() const noexcept;
	void addDefines(StringList& inList);
	void addDefine(std::string& inValue);

	bool recheck() const noexcept;
	void setRecheck(const bool inValue) noexcept;

private:
	StringList getDefaultCmakeDefines();

	StringList m_defines;
	std::string m_location;

	bool m_recheck = true;
};
}

#endif // CHALET_CMAKE_TARGET_HPP
