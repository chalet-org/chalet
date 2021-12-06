/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROFILER_RUNNER_HPP
#define CHALET_PROFILER_RUNNER_HPP

namespace chalet
{
class BuildState;
struct CommandLineInputs;
struct SourceTarget;

class ProfilerRunner
{
public:
	explicit ProfilerRunner(const CommandLineInputs& inInputs, BuildState& inState, const SourceTarget& inProject);

	bool run(const StringList& inCommand, const std::string& inExecutable);

private:
	bool runWithGprof(const StringList& inCommand, const std::string& inExecutable);
#if defined(CHALET_WIN32)
	bool runWithVisualStudioInstruments(const StringList& inCommand, const std::string& inExecutable);
#elif defined(CHALET_MACOS)
	bool runWithInstruments(const StringList& inCommand, const std::string& inExecutable, const bool inUseXcTrace);
	bool runWithSample(const StringList& inCommand, const std::string& inExecutable);
#endif

	void printExitedWithCode(const bool inResult) const;

	const CommandLineInputs& m_inputs;
	const BuildState& m_state;
	const SourceTarget& m_project;
};
}

#endif // CHALET_PROFILER_RUNNER_HPP
