/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SCRIPT_RUNNER_HPP
#define CHALET_SCRIPT_RUNNER_HPP

#include "State/ScriptType.hpp"

namespace chalet
{
struct AncillaryTools;
struct ColorTheme;
struct CommandLineInputs;

struct ScriptRunner
{
	explicit ScriptRunner(const CommandLineInputs& inInputs, const AncillaryTools& inTools);

	bool run(const std::string& inScript, const StringList& inArguments, const bool inShowExitCode);
	std::pair<StringList, ScriptType> getCommand(const std::string& inScript, const StringList& inArguments);

private:
	const CommandLineInputs& m_inputs;
	const AncillaryTools& m_tools;
	const std::string& m_inputFile;
};
}

#endif // CHALET_SCRIPT_RUNNER_HPP
