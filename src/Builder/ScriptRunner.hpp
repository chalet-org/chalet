/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SCRIPT_RUNNER_HPP
#define CHALET_SCRIPT_RUNNER_HPP

namespace chalet
{
struct AncillaryTools;

class ScriptRunner
{
public:
	explicit ScriptRunner(const AncillaryTools& inTools, const std::string& inBuildFile);

	bool run(const StringList& inScripts, const bool inShowExitCode);
	bool run(const std::string& inScript, const bool inShowExitCode);

private:
	const AncillaryTools& m_tools;
	const std::string& m_buildFile;
};
}

#endif // CHALET_SCRIPT_RUNNER_HPP
