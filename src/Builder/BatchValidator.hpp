/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"
#include "State/ScriptType.hpp"

namespace chalet
{
struct SourceCache;
class BuildState;

struct BatchValidator
{
	explicit BatchValidator(const BuildState* inState, const std::string& inSchemaFile);

	bool validate(const StringList& inFiles, const bool inCache = true);

private:
	bool parse(Json& outJson, const std::string& inFilename, const bool inPrintValid) const;
	void showErrorMessage(const std::string& inMessage) const;

	const BuildState* m_state = nullptr;
	const std::string& m_schemaFile;
};
}
