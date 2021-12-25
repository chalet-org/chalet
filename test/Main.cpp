#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include "TestState.hpp"

int main(const int argc, const char* argv[])
{
	if (!chalet::setChaletPath(argc, argv))
		return EXIT_FAILURE;

	return Catch::Session().run(argc, argv);
}