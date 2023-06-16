/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SUB_CHALET_BUILDER_HPP
#define CHALET_SUB_CHALET_BUILDER_HPP

namespace chalet
{
class BuildState;
struct SubChaletTarget;

class SubChaletBuilder
{
public:
	explicit SubChaletBuilder(const BuildState& inState, const SubChaletTarget& inTarget, const bool inQuotedPaths = false);

	std::string getBuildFile() const;
	bool dependencyHasUpdate() const;

	bool run();

	StringList getBuildCommand(const std::string& inTarget, const bool hasSettings = true) const;
	StringList getBuildCommand(const std::string& inLocation, const std::string& inBuildFile, const std::string& inTarget, const bool hasSettings = true) const;

private:
	std::string getLocation() const;
	std::string getOutputLocation() const;
	std::string getQuotedPath(const std::string& inPath) const;

	const BuildState& m_state;
	const SubChaletTarget& m_target;

	std::string m_outputLocation;

	bool m_quotedPaths = false;
};
}

#endif // CHALET_SUB_CHALET_BUILDER_HPP
