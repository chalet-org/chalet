/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PLATFORM_DEPENDENCY_MANAGER_HPP
#define CHALET_PLATFORM_DEPENDENCY_MANAGER_HPP

namespace chalet
{
class BuildState;

struct PlatformDependencyManager
{
	explicit PlatformDependencyManager(const BuildState& inState);

	void addRequiredPlatformDependency(const std::string& inKind, std::string&& inValue);
	void addRequiredPlatformDependency(const std::string& inKind, StringList&& inValue);

	bool hasRequired();

private:
	const BuildState& m_state;

	Dictionary<StringList> m_platformRequires;
};
}

#endif // CHALET_PLATFORM_DEPENDENCY_MANAGER_HPP
