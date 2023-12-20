/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/ScriptType.hpp"

namespace chalet
{
struct AncillaryTools;

struct ScriptAdapter
{
	explicit ScriptAdapter(const AncillaryTools& inTools);

	struct PathResult
	{
		ScriptType type = ScriptType::None;
		std::string file;
	};

	PathResult getScriptTypeFromPath(const std::string& inScript, const std::string& inInputFile) const;

	const std::string& getExecutable(const ScriptType inType) const;

private:
	std::string readShebangFromFile(const std::string& inFile) const;

	const AncillaryTools& m_tools;

	mutable std::unordered_map<ScriptType, std::string> m_executables;
};
}
