/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_APPLICATION_HPP
#define CHALET_APPLICATION_HPP

#include "State/CommandLineInputs.hpp"

namespace chalet
{
class Application
{
	enum class Status : ushort
	{
		Success,
		Failure
	};

public:
	Application() = default;

	int run(const int argc = 0, const char* const argv[] = nullptr);

	bool runRouteConductor();

private:
	bool initialize();
	void configureOsTerminal();
	std::string getMakeExecutable(const std::string& inCompilerPath);

	int onExit(const Status inStatus);
	// void cleanup();

	void testSignalHandling();

	CommandLineInputs m_inputs;

	bool m_initialized = false;
};
}

#endif // CHALET_APPLICATION_HPP
