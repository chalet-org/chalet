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
struct SourceTarget;

struct XcodeProjectSpecGen
{
	explicit XcodeProjectSpecGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inExportDir);

	bool saveToFile(const std::string& inFilename);

private:
	Dictionary<std::string> getConfigSettings(const BuildState& inState, const std::string& inTarget);

	const SourceTarget* getProjectFromStateContext(const BuildState& inState, const std::string& inName) const;

	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_exportDir;

	HeapDictionary<SourceOutputs> m_outputs;
};
}

#endif // CHALET_XCODE_PROJECT_SPEC_GEN_HPP
