/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SCRIPT_RUNNER_HPP
#define CHALET_SCRIPT_RUNNER_HPP

#include "CacheJson/CacheTools.hpp"

namespace chalet
{
class ScriptRunner
{
public:
	explicit ScriptRunner(const CacheTools& inTools, const bool inCleanOutput);

	bool run(const StringList& inScripts);
	bool run(const std::string& inScript);

private:
	const CacheTools& m_tools;

	const bool m_cleanOutput;
};
}

#endif // CHALET_SCRIPT_RUNNER_HPP
