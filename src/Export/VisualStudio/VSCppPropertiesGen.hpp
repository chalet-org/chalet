/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VS_CPP_PROPERTIES_GEN_HPP
#define CHALET_VS_CPP_PROPERTIES_GEN_HPP

#include "Core/Arch.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;

struct VSCppPropertiesGen
{
	explicit VSCppPropertiesGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inCwd);

	bool saveToFile(const std::string& inFilename) const;

private:
	std::string getVSArchitecture(Arch::Cpu inCpu) const;
	std::string getIntellisenseMode(const BuildState& inState) const;
	Json getEnvironments(const BuildState& inState) const;

	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_cwd;
};
}

#endif // CHALET_VS_CPP_PROPERTIES_GEN_HPP
