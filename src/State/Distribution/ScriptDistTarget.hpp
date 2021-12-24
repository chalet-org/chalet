/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SCRIPT_DIST_TARGET_HPP
#define CHALET_SCRIPT_DIST_TARGET_HPP

#include "State/Distribution/IDistTarget.hpp"

namespace chalet
{
struct ScriptDistTarget final : public IDistTarget
{
	explicit ScriptDistTarget(const CentralState& inCentralState);

	virtual bool initialize() final;
	virtual bool validate() final;

	const std::string& file() const noexcept;
	void setFile(std::string&& inValue) noexcept;

private:
	std::string m_file;
};
}

#endif // CHALET_SCRIPT_DIST_TARGET_HPP
