/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "System/SignalHandler.hpp"

#include <csignal>
#include <stdio.h>
#include <stdlib.h>

#include "Process/Environment.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

// Reference: https://spin.atomicobject.com/2013/01/13/exceptions-stack-traces-c/

namespace chalet
{
namespace
{
struct
{
	SignalHandler::Callback onErrorCallback = nullptr;
	std::unordered_map<i32, std::vector<SignalHandler::SignalFunc>> signalHandlers;
	bool exitCalled = false;
} state;

void printError(const std::string& inType, const std::string& inDescription)
{
	const auto boldRed = Output::getAnsiStyle(Output::theme().error);
	const auto reset = Output::getAnsiStyle(Output::theme().reset);
	auto output = fmt::format("{}Signal: {}{} [{}]\n", reset, inDescription, boldRed, inType);

	Output::getErrStream().write(output.data(), output.size());
}

void signalHandlerInternal(i32 inSignal)
{
	if (state.signalHandlers.find(inSignal) != state.signalHandlers.end())
	{
		bool exitHandlerCalled = false;

		auto& vect = state.signalHandlers.at(inSignal);
		for (auto& listener : vect)
		{
			if (listener == SignalHandler::exitHandler)
			{
				// We want to call this last, so skip it in this loop
				exitHandlerCalled = true;
				break;
			}
			listener(inSignal);
		}

		if (exitHandlerCalled)
		{
			SignalHandler::exitHandler(inSignal);
			::exit(1);
		}
	}
}
}

/*****************************************************************************/
void SignalHandler::add(i32 inSignal, SignalFunc inListener)
{
	if (state.signalHandlers.find(inSignal) != state.signalHandlers.end())
	{
		auto& vect = state.signalHandlers.at(inSignal);
		for (auto& listener : vect)
		{
			if (listener == inListener)
				return;
		}
		vect.emplace_back(inListener);
	}
	else
	{
		std::vector<SignalHandler::SignalFunc> vect;
		vect.emplace_back(inListener);
		state.signalHandlers.emplace(inSignal, std::move(vect));
	}
}

/*****************************************************************************/
void SignalHandler::remove(i32 inSignal, SignalFunc inListener)
{
	if (state.signalHandlers.find(inSignal) != state.signalHandlers.end())
	{
		auto& vect = state.signalHandlers.at(inSignal);
		auto it = vect.end();
		while (it != vect.begin())
		{
			--it;
			SignalFunc func = (*it);
			if (func == inListener)
			{
				it = vect.erase(it);
			}
		}
	}
}

/*****************************************************************************/
void SignalHandler::cleanup()
{
	state.signalHandlers.clear();
}

/*****************************************************************************/
void SignalHandler::start(Callback inOnError)
{
	state.onErrorCallback = inOnError;

#if defined(CHALET_DEBUG)
	SignalHandler::add(SIGABRT, SignalHandler::exitHandler);
	SignalHandler::add(SIGFPE, SignalHandler::exitHandler);
	SignalHandler::add(SIGILL, SignalHandler::exitHandler);
	SignalHandler::add(SIGINT, SignalHandler::exitHandler);
	SignalHandler::add(SIGSEGV, SignalHandler::exitHandler);
	SignalHandler::add(SIGTERM, SignalHandler::exitHandler);
#endif

	::signal(SIGABRT, signalHandlerInternal);
	::signal(SIGFPE, signalHandlerInternal);
	::signal(SIGILL, signalHandlerInternal);
	::signal(SIGINT, signalHandlerInternal);
	::signal(SIGSEGV, signalHandlerInternal);
	::signal(SIGTERM, signalHandlerInternal);
}

/*****************************************************************************/
void SignalHandler::exitHandler(const i32 inSignal)
{
	bool exceptionThrown = std::current_exception() != nullptr;
	bool assertionFailure = Diagnostic::assertionFailure();

	const auto boldRed = Output::getAnsiStyle(Output::theme().error);
	const auto reset = Output::getAnsiStyle(Output::theme().reset);

	auto& errStream = Output::getErrStream();
	errStream.put('\n');
	errStream.write(boldRed.data(), boldRed.size());
	switch (inSignal)
	{
		case SIGABRT: {
			if (exceptionThrown)
				printError("SIGABRT", "Exception Thrown");
			else if (assertionFailure)
				printError("SIGABRT", "Assertion Failure");
			else
				printError("SIGABRT", "Abort");
			break;
		}

		case SIGFPE:
			printError("SIGFPE", "Floating Point Exception (such as divide by zero)");
			break;

		case SIGILL:
			printError("SIGILL", "Illegal Instruction");
			break;

		case SIGINT:
			printError("SIGINT", "Terminal Interrupt");
			break;

		case SIGSEGV:
			printError("SIGSEGV", "Segmentation Fault");
			break;

		case SIGTERM:
			printError("SIGTERM", "Termination Request");
			break;

		default: {
			auto output = fmt::format("Unknown Signal {}:\n", inSignal);
			errStream.write(output.data(), output.size());
			break;
		}
	}

	if (state.onErrorCallback != nullptr)
		state.onErrorCallback();

	std::cout.write(reset.data(), reset.size());
	std::cout.flush();

	errStream.write(reset.data(), reset.size());
	errStream.write("\n", 1);
	errStream.flush();

	state.exitCalled = true;
}
}
