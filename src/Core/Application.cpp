/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/Application.hpp"

#include "Router/Router.hpp"

#include "Arguments/CommandLine.hpp"
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
	initializeTerminal();

	bool commandLineRead = false;
	m_inputs = CommandLine::read(argc, argv, commandLineRead);
	if (!commandLineRead)
	{
		if (m_inputs == nullptr)
			m_inputs = std::make_unique<CommandLineInputs>();
		return onExit(Status::EarlyFailure);
	}

	if (m_inputs->route().isHelp())
		return onExit(Status::Success);

	if (!handleRoute())
		return onExit(Status::Failure);

	return onExit(Status::Success);
}

/*****************************************************************************/
bool Application::handleRoute()
{
#if !defined(CHALET_DEBUG)
	CHALET_TRY
#endif
	{
		Router routes(*m_inputs);
		return routes.run();
	}
#if !defined(CHALET_DEBUG)
	CHALET_CATCH(const std::exception& err)
	{
		Diagnostic::error("Uncaught exception: {}", err.what());
		return false;
	}
#endif
}

/*****************************************************************************/
void Application::initializeTerminal()
{
#if defined(CHALET_WIN32)
	WindowsTerminal::initialize();
#endif

	std::ios::sync_with_stdio(false);
	// std::cin.tie(nullptr);

	SignalHandler::start([this]() noexcept {
		Diagnostic::printErrors();
		this->cleanup();
	});
}

/*****************************************************************************/
int Application::onExit(const Status inStatus)
{
	chalet_assert(m_inputs != nullptr, "m_inputs must be allocated.");
	if (inStatus == Status::EarlyFailure && m_inputs != nullptr)
	{
		ThemeSettingsJsonParser themeParser(*m_inputs);
		UNUSED(themeParser.serialize());
	}
	m_inputs.reset();

	Diagnostic::printErrors();

	this->cleanup();

	int result = static_cast<int>(inStatus);

	return result > 0 ? 1 : 0;
}

/*****************************************************************************/
void Application::cleanup()
{
	SignalHandler::cleanup();

#if defined(CHALET_WIN32)
	WindowsTerminal::cleanup();
#endif
}

}
