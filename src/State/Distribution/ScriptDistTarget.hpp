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
	explicit ScriptDistTarget();

	virtual bool initialize(const BuildState& inState) final;
	virtual bool validate() final;

	const StringList& scripts() const noexcept;
	void addScripts(StringList&& inList);
	void addScript(std::string&& inValue);

private:
	StringList m_scripts;
};
}

#endif // CHALET_SCRIPT_DIST_TARGET_HPP
