/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;

struct MacosNotarizationMsg
{
	explicit MacosNotarizationMsg(const BuildState& inState);

	void showMessage(const std::string& inFile);

private:
	const BuildState& m_state;
};
}
