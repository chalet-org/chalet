/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DEPENDENCY_MANAGER_HPP
#define CHALET_DEPENDENCY_MANAGER_HPP

namespace chalet
{
struct StatePrototype;

class DependencyManager
{
public:
	explicit DependencyManager(StatePrototype& inPrototype);

	bool run(const bool inConfigureCmd);

private:
	StatePrototype& m_prototype;
};
}

#endif // CHALET_DEPENDENCY_MANAGER_HPP
