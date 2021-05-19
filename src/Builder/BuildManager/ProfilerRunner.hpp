/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROFILER_RUNNER_HPP
#define CHALET_PROFILER_RUNNER_HPP

#include "BuildJson/ProjectConfiguration.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
class ProfilerRunner
{
public:
	explicit ProfilerRunner(BuildState& inState, const ProjectConfiguration& inProject, const bool inCleanOutput);

	bool run(const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder);

private:
	bool runWithGprof(const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder);
#if defined(CHALET_MACOS)
	bool runWithInstruments(const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder, const bool inUseXcTrace);
	bool runWithSample(const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder);
#endif

	const BuildState& m_state;
	const ProjectConfiguration& m_project;

	const bool m_cleanOutput;
};
}

#endif // CHALET_PROFILER_RUNNER_HPP
