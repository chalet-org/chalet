/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/SignalHandler.hpp"

#include "Libraries/StackTrace.hpp"

#include <csignal>
#include <stdio.h>
#include <stdlib.h>

#include "Terminal/Color.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/Reflect.hpp"
#include "Utility/String.hpp"

// Reference: https://spin.atomicobject.com/2013/01/13/exceptions-stack-traces-c/

namespace chalet::priv
{
/*****************************************************************************/
SignalHandler::Callback SignalHandler::sOnErrorCallback = nullptr;

/*****************************************************************************/
void SignalHandler::start(Callback inOnError)
{
	sOnErrorCallback = inOnError;

	std::signal(SIGABRT, SignalHandler::handler);
	std::signal(SIGFPE, SignalHandler::handler);
	std::signal(SIGILL, SignalHandler::handler);
	std::signal(SIGINT, SignalHandler::handler);
	std::signal(SIGSEGV, SignalHandler::handler);
	std::signal(SIGTERM, SignalHandler::handler);
}

/*****************************************************************************/
void SignalHandler::handler(const int inSignal)
{
	bool exceptionThrown = std::current_exception() != nullptr;
	bool assertionFailure = Diagnostic::assertionFailure();

	const auto boldRed = Output::getAnsiStyle(Color::Red, true);
	const auto reset = Output::getAnsiReset();

	std::cerr << "\n"
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
			printError("SIGINT", "Terminal Interrupt", false);
			break;

		case SIGSEGV:
			printError("SIGSEGV", "Segmentation Fault");
			break;

		case SIGTERM:
			printError("SIGTERM", "Termination Request");
			break;

		default:
			std::cerr << "Unknown Signal " + std::to_string(inSignal) + ":\n";
			printStackTrace();
			break;
	}

	std::cout << reset << std::flush;
	std::cerr << reset << std::endl;

	if (sOnErrorCallback != nullptr)
		sOnErrorCallback();

	std::exit(1);
}

/*****************************************************************************/
void SignalHandler::printError(const std::string& inType, const std::string& inDescription, const bool inPrintStackTrace)
{
	const auto boldRed = Output::getAnsiStyle(Color::Red, true);
	std::cerr << boldRed << inType + "\n";

	if (!inDescription.empty())
		std::cerr << inDescription + ":\n";

	if (inPrintStackTrace)
		printStackTrace();
}

/*****************************************************************************/
void SignalHandler::printStackTrace()
{
	std::string workingDirectory = Commands::getWorkingDirectory();
	if (workingDirectory.empty())
		return;

	Path::sanitize(workingDirectory);

	const auto boldRed = Output::getAnsiStyle(Color::Red, true);
	const auto boldBlack = Output::getAnsiStyle(Color::Black, true);
	const auto redHighlight = Output::getAnsiStyle(Color::Reset, Color::Red, true);
	const auto blue = Output::getAnsiStyle(Color::Blue);
	const auto reset = Output::getAnsiReset();

	auto thisClassName = CHALET_REFLECT(SignalHandler);
	auto diagnosticClassName = CHALET_REFLECT(Diagnostic);

	bool highlight = true;
	ust::StackTrace stacktrace = ust::generate();
	for (auto& entry : stacktrace.entries)
	{
		// ignore this class
		if (String::contains(thisClassName, entry.functionName) || String::contains(diagnosticClassName, entry.functionName))
			continue;

		// TODO: Is there a way to get the terminal width?
		if (entry.functionName.size() > 100)
			entry.functionName = entry.functionName.substr(0, 100) + "...";

#if !defined(CHALET_MACOS)
		// note: entry.sourceFileName will be blank if there are no debugging symbols!
		std::size_t pos = entry.sourceFileName.find(workingDirectory);
		const bool sourceCode = pos != std::string::npos;
		std::string sourceFile = sourceCode ? entry.sourceFileName.substr(pos + workingDirectory.length() + 1) : entry.sourceFileName;
#else
		const bool sourceCode = !entry.sourceFileName.empty();
		std::string sourceFile = entry.sourceFileName;
#endif
		// Note: breakpoints vary depending on compiler optimization level
		if (sourceCode)
		{
			// at TestScene::init() src/main/Scenes/TestScene.cpp:42
			if (highlight)
			{
				std::cerr << redHighlight << "  at";
				highlight = false;
			}
			else
			{
				std::cerr << boldRed << "  at";
			}

			std::cerr << reset << " " << entry.functionName << " " << blue << sourceFile << ":" << entry.lineNumber << reset << "\n";
		}
		// OS dynamic libs, etc
		else if (entry.functionName.empty())
		{
			// at C:/Windows/System32/msvcrt.dll:0x7ff8f04e7c58
			// Skip these, because they're just noise (until they're not)
			// std::cout << boldRed << "  at " << boldBlack << entry.binaryFileName << ":" << entry.address << reset << "\n";
		}
		// C++ runtime, libstdc++, libgcc, etc.
		else
		{
			// at mainCRTStartup
			std::cerr << boldRed << "  at " << boldBlack << entry.functionName << " " << entry.lineNumber << reset << "\n";
		}
	}
}
}
