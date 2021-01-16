#include "TestCase.hpp"

TEST_CASE("chalet::SimpleTest", "[test]")
{
	int i = 1;
	float j = 1.0f;

	REQUIRE(static_cast<int>(j) == i);
}