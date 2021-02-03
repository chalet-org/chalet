/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SIGNAL_HANDLER_HPP
#define CHALET_SIGNAL_HANDLER_HPP

namespace chalet::priv
{
class SignalHandler
{
	using Callback = std::function<void()>;

public:
	static void start(Callback inOnError = nullptr);
	static void printStackTrace();

private:
	SignalHandler() = delete;

	static void handler(const int inSignal);

	static void printError(const std::string& inType, const std::string& inDescription, const bool inPrintStackTrace = true);

	static Callback sOnErrorCallback;
};
}

#endif // CHALET_SIGNAL_HANDLER_HPP