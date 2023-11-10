#include "TestCase.hpp"

namespace chalet
{
TEST_CASE("chalet::SimpleTest", "[test]")
{
	i32 i = 1;
	f32 j = 1.0f;

	REQUIRE(static_cast<i32>(j) == i);
}
}
