/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SCRIPT_BUILD_TARGET_HPP
#define CHALET_SCRIPT_BUILD_TARGET_HPP

#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
struct ScriptBuildTarget final : public IBuildTarget
{
	explicit ScriptBuildTarget(const BuildState& inState);

	virtual void initialize() final;
	virtual bool validate() final;

	const StringList& scripts() const noexcept;
	void addScripts(StringList&& inList);
	void addScript(std::string&& inValue);

private:
	StringList m_scripts;
};
}

#endif // CHALET_SCRIPT_BUILD_TARGET_HPP
