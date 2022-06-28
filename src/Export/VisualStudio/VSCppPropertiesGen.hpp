/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VS_CPP_PROPERTIES_GEN_HPP
#define CHALET_VS_CPP_PROPERTIES_GEN_HPP

namespace chalet
{
class BuildState;

struct VSCppPropertiesGen
{
	explicit VSCppPropertiesGen(const BuildState& inState, const std::string& inCwd, const std::string& inDebugConfiguration);

	bool saveToFile(const std::string& inFilename) const;

private:
	const BuildState& m_state;
	const std::string& m_cwd;
	const std::string& m_debugConfiguration;
};
}

#endif // CHALET_VS_CPP_PROPERTIES_GEN_HPP
