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

	virtual void initialize() final;
	virtual bool validate() final;

	const std::string& location() const noexcept;
	void setLocation(std::string&& inValue) noexcept;

	bool recheck() const noexcept;
	void setRecheck(const bool inValue) noexcept;

private:
	std::string m_location;

	bool m_recheck = true;
};
}

#endif // CHALET_SUB_CHALET_TARGET_HPP
