/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_XCODE_PROJECT_SPEC_GEN_HPP
#define CHALET_XCODE_PROJECT_SPEC_GEN_HPP

#include "State/SourceOutputs.hpp"

namespace chalet
{
class BuildState;

struct XcodeProjectSpecGen
{
	explicit XcodeProjectSpecGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inExportDir);

	bool saveToFile(const std::string& inFilename) const;

private:
	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_exportDir;
};
}

#endif // CHALET_XCODE_PROJECT_SPEC_GEN_HPP
