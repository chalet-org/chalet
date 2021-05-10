/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/Application.hpp"

#include "Router/Router.hpp"

#include "Core/ArgumentParser.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/SignalHandler.hpp"

namespace chalet
{
/*****************************************************************************/
int Application::run(const int argc, const char* const argv[])
{
	if (!initialize())
	{
		Diagnostic::error("Cannot call 'Application::run' more than once.");
		return onExit(Status::Failure);
	}

	{
		ArgumentParser argParser{ m_inputs };
		if (!argParser.run(argc, argv))
			return onExit(Status::Success);
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

	m_osTerminal.initialize();

#ifdef CHALET_DEBUG
	priv::SignalHandler::start([this]() noexcept {
		this->cleanup();
	});
	testSignalHandling();
#endif // _DEBUG

	m_initialized = true;

	return true;
}

/*****************************************************************************/
int Application::onExit(const Status inStatus)
{
	cleanup();

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
void Application::cleanup()
{
	m_osTerminal.cleanup();
}

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
