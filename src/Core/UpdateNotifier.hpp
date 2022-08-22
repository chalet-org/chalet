/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_UPDATE_NOTIFIER_HPP
#define CHALET_UPDATE_NOTIFIER_HPP

namespace chalet
{
struct CentralState;

struct UpdateNotifier
{
	explicit UpdateNotifier(const CentralState& inCentralState);

	void notifyForUpdates();

private:
	void showUpdateMessage(const std::string& inOld, const std::string& inNew) const;

	const CentralState& m_centralState;
};
}

#endif // CHALET_UPDATE_NOTIFIER_HPP
