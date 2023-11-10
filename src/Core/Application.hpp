/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Core/CommandLineInputs.hpp"

namespace chalet
{
class Application
{
	enum class Status
	{
		Success = EXIT_SUCCESS,
		Failure = EXIT_FAILURE,
		EarlyFailure = EXIT_FAILURE + 1
	};

public:
	Application() = default;

	i32 run(const i32 argc = 0, const char* argv[] = nullptr);

private:
	void initializeTerminal();
	bool handleRoute();

	i32 onExit(const Status inStatus);
	void cleanup();

	Unique<CommandLineInputs> m_inputs;
};
}
