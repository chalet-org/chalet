/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct CentralState;

struct UpdateNotifier
{
	static void checkForUpdates(const CentralState& inCentralState);

private:
	explicit UpdateNotifier(const CentralState& inCentralState);

	void checkForUpdates();
	void showUpdateMessage(const std::string& inOld, const std::string& inNew) const;

	const CentralState& m_centralState;
};
}
