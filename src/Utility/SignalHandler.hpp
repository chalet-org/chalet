/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SIGNAL_HANDLER_HPP
#define CHALET_SIGNAL_HANDLER_HPP

namespace chalet::priv
{
namespace SignalHandler
{
using Callback = std::function<void()>;

void start(Callback inOnError = nullptr);
void handler(const int inSignal);
}
}

#endif // CHALET_SIGNAL_HANDLER_HPP