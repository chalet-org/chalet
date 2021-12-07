/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SCRIPT_RUNNER_HPP
#define CHALET_SCRIPT_RUNNER_HPP

namespace chalet
{
struct AncillaryTools;
struct ColorTheme;
struct CommandLineInputs;

class ScriptRunner
{
public:
	explicit ScriptRunner(const CommandLineInputs& inInputs, const AncillaryTools& inTools);

	bool run(const StringList& inScripts, const bool inShowExitCode);
	bool run(const std::string& inScript, const bool inShowExitCode);

private:
	const CommandLineInputs& m_inputs;
	const AncillaryTools& m_tools;
	const std::string& m_inputFile;
};
}

#endif // CHALET_SCRIPT_RUNNER_HPP
