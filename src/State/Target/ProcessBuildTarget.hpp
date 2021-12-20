/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROCESS_BUILD_TARGET_HPP
#define CHALET_PROCESS_BUILD_TARGET_HPP

#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
struct ProcessBuildTarget final : public IBuildTarget
{
	explicit ProcessBuildTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;

	const std::string& path() const noexcept;
	void setPath(std::string&& inValue) noexcept;

private:
	std::string m_path;
};
}

#endif // CHALET_SCRIPT_BUILD_TARGET_HPP
