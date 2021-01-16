#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

int main(const int argc, const char* argv[])
{
	return Catch::Session().run(argc, argv);
}