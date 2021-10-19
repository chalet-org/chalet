/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/SignalHandler.hpp"

#include <csignal>
#include <stdio.h>
#include <stdlib.h>

#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/Reflect.hpp"
#include "Utility/String.hpp"

// Reference: https://spin.atomicobject.com/2013/01/13/exceptions-stack-traces-c/

namespace chalet::priv
{
namespace
{
static struct
{
	SignalHandler::Callback onErrorCallback = nullptr;
} state;

void printError(const std::string& inType, const std::string& inDescription)
{
	const auto boldRed = Output::getAnsiStyle(Output::theme().error);
	std::cerr << boldRed << inType + '\n';

	if (!inDescription.empty())
	{
		std::cerr << inDescription + ":\n";
	}
}

}
/*****************************************************************************/
void SignalHandler::start(Callback inOnError)
{
	state.onErrorCallback = inOnError;

	::signal(SIGABRT, SignalHandler::handler);
	::signal(SIGFPE, SignalHandler::handler);
	::signal(SIGILL, SignalHandler::handler);
	::signal(SIGINT, SignalHandler::handler);
	::signal(SIGSEGV, SignalHandler::handler);
	::signal(SIGTERM, SignalHandler::handler);
}

/*****************************************************************************/
void SignalHandler::handler(const int inSignal)
{
	bool exceptionThrown = std::current_exception() != nullptr;
	bool assertionFailure = Diagnostic::assertionFailure();

	const auto boldRed = Output::getAnsiStyle(Output::theme().error);
	const auto reset = Output::getAnsiStyle(Color::Reset);

	std::cerr << '\n'
			  << boldRed;
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

		default:
			std::cerr << "Unknown Signal " + std::to_string(inSignal) + ":\n";
			break;
	}

	if (state.onErrorCallback != nullptr)
		state.onErrorCallback();

	std::cout << reset << std::flush;
	std::cerr << reset << std::endl;

	::exit(1);
}
}
