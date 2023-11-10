/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;

struct VSCodeCCppPropertiesGen
{
	explicit VSCodeCCppPropertiesGen(const BuildState& inState);

	bool saveToFile(const std::string& inFilename) const;

private:
	std::string getName() const;
	std::string getIntellisenseMode() const;
	std::string getCompilerPath() const;
	StringList getDefaultDefines() const;

	const BuildState& m_state;
};
}
