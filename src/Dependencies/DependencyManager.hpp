/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DEPENDENCY_MANAGER_HPP
#define CHALET_DEPENDENCY_MANAGER_HPP

#include "State/BuildState.hpp"

namespace chalet
{
class DependencyManager
{
public:
	explicit DependencyManager(BuildState& inState, const bool inCleanOutput);

	bool run(const bool inInstallCmd);

private:
	BuildState& m_state;

	const bool m_cleanOutput;
};
}

#endif // CHALET_DEPENDENCY_MANAGER_HPP
