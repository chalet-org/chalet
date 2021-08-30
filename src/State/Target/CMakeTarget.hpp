/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CMAKE_TARGET_HPP
#define CHALET_CMAKE_TARGET_HPP

#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
struct CMakeTarget final : public IBuildTarget
{
	explicit CMakeTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;

	const StringList& defines() const noexcept;
	void addDefines(StringList&& inList);
	void addDefine(std::string&& inValue);

	const std::string& location() const noexcept;
	void setLocation(std::string&& inValue) noexcept;

	const std::string& buildFile() const noexcept;
	void setBuildFile(std::string&& inValue) noexcept;

	const std::string& toolset() const noexcept;
	void setToolset(std::string&& inValue) noexcept;

	bool recheck() const noexcept;
	void setRecheck(const bool inValue) noexcept;

private:
	StringList m_defines;
	std::string m_location;
	std::string m_buildFile;
	std::string m_toolset;

	bool m_recheck = true;
};
}

#endif // CHALET_CMAKE_TARGET_HPP
