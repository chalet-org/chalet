/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SCRIPT_TARGET_HPP
#define CHALET_SCRIPT_TARGET_HPP

#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
struct ScriptTarget final : public IBuildTarget
{
	explicit ScriptTarget(const BuildState& inState);

	const StringList& scripts() const noexcept;
	void addScripts(StringList& inList);
	void addScript(std::string& inValue);

private:
	StringList m_scripts;
};
}

#endif // CHALET_SCRIPT_TARGET_HPP
