/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WORKSPACE_ENVIRONMENT_HPP
#define CHALET_WORKSPACE_ENVIRONMENT_HPP

#include "Utility/Version.hpp"

namespace chalet
{
struct TargetMetadata;
class BuildState;

struct WorkspaceEnvironment
{
	WorkspaceEnvironment();

	bool initialize(const BuildState& inPaths);

	const TargetMetadata& metadata() const noexcept;
	void setMetadata(Ref<TargetMetadata>&& inValue);

	const StringList& searchPaths() const noexcept;
	void addSearchPaths(StringList&& inList);
	void addSearchPath(std::string&& inValue);
	std::string makePathVariable(const std::string& inRootPath) const;
	std::string makePathVariable(const std::string& inRootPath, const StringList& inAdditionalPaths) const;

private:
	StringList m_searchPaths;

	Ref<TargetMetadata> m_metadata;
};
}

#endif // CHALET_WORKSPACE_ENVIRONMENT_HPP
