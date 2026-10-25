/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"
#include "State/ScriptType.hpp"

namespace chalet
{
class BuildState;
struct SourceTarget;

struct RenderDocCaptureSettingsGen
{
	explicit RenderDocCaptureSettingsGen(const BuildState& inState, const SourceTarget& inProject);

	bool generate();

private:
	Json getEnvironmentAsJson() const;
	std::string getRunArguments() const;

	const BuildState& m_state;
	const SourceTarget& m_project;
};
}
