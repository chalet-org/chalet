/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;

struct XcodeXSchemeGen
{
	XcodeXSchemeGen(std::vector<Unique<BuildState>>& inStates, const std::string& inXcodeProj, const std::string& inDebugConfig);

	bool createSchemes(const std::string& inSchemePath);

private:
	std::string getTargetHash(const std::string& inTarget) const;
	std::string getBoolString(const bool inValue) const;

	std::vector<Unique<BuildState>>& m_states;
	const std::string& m_xcodeProj;
	const std::string& m_debugConfiguration;
	std::string m_xcodeNamespaceGuid;
};
}
