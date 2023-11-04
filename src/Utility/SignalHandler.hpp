/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SIGNAL_HANDLER_HPP
#define CHALET_SIGNAL_HANDLER_HPP

namespace chalet
{
namespace SignalHandler
{
using Callback = std::function<void()>;
using SignalFunc = void (*)(int);

void add(int inSignal, SignalFunc inListener);
void cleanup();

void start(Callback inOnError = nullptr);
void exitHandler(const int inSignal);
}
}

#endif // CHALET_SIGNAL_HANDLER_HPP