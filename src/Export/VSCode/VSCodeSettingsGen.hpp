/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Export/VSCode/VSCodeExtensionAwarenessAdapter.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;

struct VSCodeSettingsGen
{
	VSCodeSettingsGen(const BuildState& inState, const VSCodeExtensionAwarenessAdapter& inExtensionAdapter);

	bool saveToFile(const std::string& inFilename);

private:
	std::string getRemoteSchemaPath(const std::string& inFile) const;

	void setFormatOnSave(Json& outJson) const;
	void setFallbackSchemaSettings(Json& outJson) const;

	const BuildState& m_state;
	const VSCodeExtensionAwarenessAdapter& m_extensionAdapter;
};
}
