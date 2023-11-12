/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
namespace SignalHandler
{
using Callback = std::function<void()>;
using SignalFunc = void (*)(i32);

void add(i32 inSignal, SignalFunc inListener);
void remove(i32 inSignal, SignalFunc inListener);
void cleanup();

void start(Callback inOnError = nullptr);
void exitHandler(const i32 inSignal);
}
}
