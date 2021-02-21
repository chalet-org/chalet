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

		// Output::resetStdout();
		// Output::resetStderr();

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

		// SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_VIRTUAL_TERMINAL_PROCESSING);
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hOut != INVALID_HANDLE_VALUE)
		{
			DWORD dwMode = 0;
			if (GetConsoleMode(hOut, &dwMode))
			{
				dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
				SetConsoleMode(hOut, dwMode);
			}
		}
	}

	{
		// auto result = std::system("cmd -v");
		// LOG("GetConsoleScreenBufferInfo", result);
		// UNUSED(result);
	}

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	if (Environment::isMsvc())
	{
		// Save the current environment to a file
		// std::system("SET > build/all_variables.txt");

		// auto visualStudioPath = Environment::get("VSAPPIDDIR");
		// LOG(visualStudioPath);
		// if (visualStudioPath != nullptr)
		// {
		// 	// 32-bit arch would use vcvars32.bat
		// 	std::system(fmt::format("\"{}..\\..\\VC\\Auxiliary\\Build\\vcvars64.bat\" > nul && SET > build/all_variables_msvc.txt", visualStudioPath).c_str());
		// }
	}
	else
#endif
	{
		// Save the current environment to a file
		// std::system("printenv > build/all_variables.txt");

		Environment::set("GCC_COLORS", "error=01;31:warning=01;33:note=01;36:caret=01;32:locus=00;34:quote=01");
	}

	// auto path = Environment::getPath();
	// LOG(path);
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
		// Commands::sleep(3.0);
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
	// int test2 = *test;					// SIGSEGV / segmentation fault

	// int a = 0;
	// int test2 = 25 / a; // SIGFPE / arithmetic error

	// LOG(test2);

	// ::raise(SIGILL);
	// ::raise(SIGTERM);
	// ::raise(SIGABRT); // generic abort
}
}
