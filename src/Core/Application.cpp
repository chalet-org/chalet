/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/Application.hpp"

#include "Router/Router.hpp"

#include "Arguments/ArgumentReader.hpp"
#include "SettingsJson/ThemeSettingsJsonParser.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/SignalHandler.hpp"

#if defined(CHALET_WIN32)
	#include "Terminal/WindowsTerminal.hpp"
#endif

namespace chalet
{
/*****************************************************************************/
int Application::run(const int argc, const char* argv[])
{
	initialize();

	{
		ArgumentReader argParser{ m_inputs };
		if (!argParser.run(argc, argv))
			return onExit(Status::EarlyFailure);
	}

	if (m_inputs.route() == Route::Help)
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
void Application::initialize()
{
	// Output::resetStdout();
	// Output::resetStderr();

#if defined(CHALET_WIN32)
	WindowsTerminal::initialize();
#endif

	std::ios::sync_with_stdio(false);
	// std::cin.tie(nullptr);

#if defined(CHALET_DEBUG)
	priv::SignalHandler::start([this]() noexcept {
		Diagnostic::printErrors();
		this->cleanup();
	});
	testSignalHandling();
#endif // _DEBUG
}

/*****************************************************************************/
int Application::onExit(const Status inStatus)
{
	if (inStatus == Status::EarlyFailure)
	{
		ThemeSettingsJsonParser themeParser(m_inputs);
		UNUSED(themeParser.serialize());
	}

	Diagnostic::printErrors();

	this->cleanup();

	int result = static_cast<int>(inStatus);

	return result > 0 ? 1 : 0;
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
