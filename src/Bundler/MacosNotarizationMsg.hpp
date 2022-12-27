/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MACOS_NOTARIZATION_MSG_HPP
#define CHALET_MACOS_NOTARIZATION_MSG_HPP

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

#endif // CHALET_MACOS_NOTARIZATION_MSG_HPP
