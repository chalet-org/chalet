/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_C_CPP_PROPERTIES_GEN_HPP
#define CHALET_C_CPP_PROPERTIES_GEN_HPP

namespace chalet
{
class BuildState;

struct CCppPropertiesGen
{
	explicit CCppPropertiesGen(const BuildState& inState, const std::string& inCwd);

	bool saveToFile(const std::string& inFilename) const;

private:
	std::string getName() const;
	std::string getIntellisenseMode() const;
	std::string getCompilerPath() const;
	StringList getDefaultDefines() const;

	const BuildState& m_state;
	const std::string& m_cwd;
};
}

#endif // CHALET_C_CPP_PROPERTIES_GEN_HPP
