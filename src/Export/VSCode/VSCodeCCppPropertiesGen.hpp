/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Export/ExportAdapter.hpp"

namespace chalet
{
class BuildState;

struct VSCodeCCppPropertiesGen
{
	VSCodeCCppPropertiesGen(const BuildState& inState, const ExportAdapter& inExportAdapter);

	bool saveToFile(const std::string& inFilename) const;

private:
	std::string getName() const;
	std::string getIntellisenseMode() const;
	std::string getCompilerPath() const;

	const BuildState& m_state;
	const ExportAdapter& m_exportAdapter;
};
}
