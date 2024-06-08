/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/ScriptType.hpp"

namespace chalet
{
struct AncillaryTools;
struct ColorTheme;
struct CommandLineInputs;
struct SourceCache;

struct ScriptRunner
{
	explicit ScriptRunner(const CommandLineInputs& inInputs, const AncillaryTools& inTools);

	bool run(const ScriptType inType, const std::string& inScript, const StringList& inArguments, const bool inShowExitCode);
	StringList getCommand(const ScriptType inType, const std::string& inScript, const StringList& inArguments, const bool inQuotePaths);
	bool shouldRun(SourceCache& inSourceCache, const StringList& inDepends) const;

private:
	const CommandLineInputs& m_inputs;
	const AncillaryTools& m_tools;
	const std::string& m_inputFile;
};
}
