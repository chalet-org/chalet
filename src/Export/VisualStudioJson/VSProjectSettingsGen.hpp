/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Export/ExportAdapter.hpp"

namespace chalet
{
class BuildState;

struct VSProjectSettingsGen
{
	explicit VSProjectSettingsGen(const ExportAdapter& inExportAdapter);

	bool saveToFile(const std::string& inFilename);

private:
	const ExportAdapter& m_exportAdapter;
};
}
