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
	explicit DependencyManager(BuildState& inState);

	bool run(const bool inInstallCmd);

private:
	BuildState& m_state;
};
}

#endif // CHALET_DEPENDENCY_MANAGER_HPP
