/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_XCODE_XSCHEME_GEN_HPP
#define CHALET_XCODE_XSCHEME_GEN_HPP

namespace chalet
{
class BuildState;

struct XcodeXSchemeGen
{
	XcodeXSchemeGen(std::vector<Unique<BuildState>>& inStates, const std::string& inXcodeProj);

	bool createSchemes(const std::string& inSchemePath);

private:
	std::string getTargetHash(const std::string& inTarget) const;
	std::string getBoolString(const bool inValue) const;

	std::vector<Unique<BuildState>>& m_states;
	const std::string& m_xcodeProj;
	std::string m_xcodeNamespaceGuid;
};
}

#endif // CHALET_XCODE_XSCHEME_GEN_HPP