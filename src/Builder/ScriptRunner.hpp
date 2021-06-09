/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SCRIPT_RUNNER_HPP
#define CHALET_SCRIPT_RUNNER_HPP

#include "State/CacheTools.hpp"

namespace chalet
{
class ScriptRunner
{
public:
	explicit ScriptRunner(const CacheTools& inTools, const std::string& inBuildFile, const bool inCleanOutput);

	bool run(const StringList& inScripts);
	bool run(const std::string& inScript);

private:
	const CacheTools& m_tools;
	const std::string& m_buildFile;

	const bool m_cleanOutput;
};
}

#endif // CHALET_SCRIPT_RUNNER_HPP
