/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/Application.hpp"

#include <stdlib.h>
#include <thread>

#include "Router/Router.hpp"

#include "Core/ArgumentParser.hpp"
#include "Libraries/Format.hpp"
#include "Libraries/WindowsApi.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/SignalHandler.hpp"

namespace chalet
{
/*****************************************************************************/
int Application::run(const int argc, const char* const argv[])
{
	if (!initialize())
	{
		Diagnostic::errorAbort("Cannot call 'Application::run' more than once.");
		return EXIT_FAILURE;
	}

	if (!Environment::isBash())
	{
		Diagnostic::error(fmt::format("This application was designed for use with bash on Windows. Command Prompt, PowerShell & MSVC are not yet supported."), "Critical Error");
		return EXIT_SUCCESS;
	}

	{
		ArgumentParser argParser{ m_inputs };
		if (!argParser.run(argc, argv))
			return EXIT_SUCCESS;
	}

	if (!runRouteConductor())
		return onExit(Status::Failure);

	return onExit(Status::Success);
}

/*****************************************************************************/
bool Application::runRouteConductor()
{
	Router routes(m_inputs);
	return routes.run();
}

/*****************************************************************************/
bool Application::initialize()
{
	if (m_initialized)
		return false;

#ifdef CHALET_DEBUG
	priv::SignalHandler::start([]() noexcept {
		// this->cleanup();
	});
	testSignalHandling();
#endif // _DEBUG

	configureOsTerminal();

	m_initialized = true;

	return true;
}

/*****************************************************************************/
void Application::configureOsTerminal()
{
#if defined(CHALET_WIN32)
	{
		// This actually just fixes MSYS output for unicode characters. command prompt is still busted
		auto result = SetConsoleOutputCP(CP_UTF8);
		chalet_assert(result, "Failed to set Console encoding.");
		UNUSED(result);
	}

	{
		// auto result = std::system("cmd -v");
		// LOG("GetConsoleScreenBufferInfo", result);
		// UNUSED(result);
	}

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif

	Environment::set("GCC_COLORS", "error=01;31:warning=01;33:note=01;36:caret=01;32:locus=00;34:quote=01");
}

/*****************************************************************************/
int Application::onExit(const Status inStatus)
{
	// cleanup();

	if (inStatus == Status::Success)
	{
		return EXIT_SUCCESS;
	}
	else
	{
		// This exit method just means we've handled the error elsewhere and displayed it
		// std::this_thread::sleep_for(std::chrono::seconds(3));
		return EXIT_FAILURE;
	}
}

/*****************************************************************************/
/*void Application::cleanup()
{
}*/

/*****************************************************************************/
void Application::testSignalHandling()
{
	// int* test = nullptr;
	// chalet_assert(test != nullptr, ""); // SIGABRT / abort
	// int test2 = *test;				// SIGSEGV / segmentation fault

	// int a = 0;
	// int test2 = 25 / a; // SIGFPE / arithmetic error

	// LOG(test2);

	// ::raise(SIGILL);
	// ::raise(SIGTERM);
	// ::raise(SIGABRT); // generic abort
}
}
