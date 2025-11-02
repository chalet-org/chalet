/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;

struct ZedSettingsGen
{
	explicit ZedSettingsGen(const BuildState& inState);

	bool saveToFile(const std::string& inFilename) const;

private:
	const BuildState& m_state;
};
}
