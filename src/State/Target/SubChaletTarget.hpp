/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SUB_CHALET_TARGET_HPP
#define CHALET_SUB_CHALET_TARGET_HPP

#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
struct SubChaletTarget final : public IBuildTarget
{
	explicit SubChaletTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;

	const std::string& location() const noexcept;
	void setLocation(std::string&& inValue) noexcept;

	const std::string& buildFile() const noexcept;
	void setBuildFile(std::string&& inValue) noexcept;

	bool recheck() const noexcept;
	void setRecheck(const bool inValue) noexcept;

	bool rebuild() const noexcept;
	void setRebuild(const bool inValue) noexcept;

private:
	std::string m_location;
	std::string m_buildFile;

	bool m_recheck = true;
	bool m_rebuild = true;
};
}

#endif // CHALET_SUB_CHALET_TARGET_HPP
