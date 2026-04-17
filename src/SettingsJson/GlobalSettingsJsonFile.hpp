/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"
#include "Json/IJsonFileParser.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct WorkspaceCache;
struct IntermediateSettingsState;

struct GlobalSettingsJsonFile final : public IJsonFileParser
{
	static bool read(WorkspaceCache& inCache, IntermediateSettingsState& inFallback, bool& outShouldPerformUpdateCheck);

private:
	explicit GlobalSettingsJsonFile(WorkspaceCache& inCache, IntermediateSettingsState& inFallback, bool& outShouldPerformUpdateCheck);

	virtual bool deserialize() final;

	void reassignIntermediateStateFromSettings(IntermediateSettingsState& outState, const Json& inNode) const;

	bool assignDefaultTheme(Json& outNode);
	bool assignLastUpdate(Json& outNode);

	bool shouldPerformUpdateCheckBasedOnLastUpdate(const time_t inLastUpdate, const time_t inCurrent) const;

	JsonFile& m_jsonFile;

	IntermediateSettingsState& m_fallback;
	bool& m_shouldPerformUpdateCheck;
};
}
