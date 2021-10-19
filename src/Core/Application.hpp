/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_APPLICATION_HPP
#define CHALET_APPLICATION_HPP

#include "Core/CommandLineInputs.hpp"

namespace chalet
{
class Application
{
	enum class Status
	{
		Success = EXIT_SUCCESS,
		Failure = EXIT_FAILURE
	};

public:
	Application() = default;

	int run(const int argc = 0, const char* const argv[] = nullptr);

private:
	bool initialize();
	bool handleRoute();

	int onExit(const Status inStatus);
	void cleanup();

	void testSignalHandling();

	CommandLineInputs m_inputs;

	bool m_initialized = false;
};
}

#endif // CHALET_APPLICATION_HPP
