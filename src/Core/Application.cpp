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

#if defined(CHALET_WIN32)
	#include "Terminal/WindowsTerminal.hpp"
#endif

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
			return onExit(Status::Failure);
	}

	if (m_inputs.command() == Route::Help)
		return onExit(Status::Success);

	if (!handleRoute())
		return onExit(Status::Failure);

	return onExit(Status::Success);
}

/*****************************************************************************/
bool Application::handleRoute()
{
	CHALET_TRY
	{
		Router routes(m_inputs);
		return routes.run();
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR("Uncaught exception: {}", err.what());
		return false;
	}
}

/*****************************************************************************/
bool Application::initialize()
{
	if (m_initialized)
		return false;

		// Output::resetStdout();
		// Output::resetStderr();

#if defined(CHALET_WIN32)
	WindowsTerminal::initialize();
#endif

#if defined(CHALET_DEBUG)
	priv::SignalHandler::start([this]() noexcept {
		Diagnostic::printErrors();
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
	Diagnostic::printErrors();
	this->cleanup();

	return static_cast<int>(inStatus);
}

/*****************************************************************************/
void Application::cleanup()
{
#if defined(CHALET_WIN32)
	WindowsTerminal::cleanup();
#endif
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
