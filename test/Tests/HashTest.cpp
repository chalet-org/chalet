#include "TestCase.hpp"

#include "Utility/Hash.hpp"

namespace chalet
{
TEST_CASE("chalet::HashTest", "[hash]")
{
	std::string val{ "Build all of the c++ code." };

	REQUIRE(Hash::string(val) == "d23987548777d0e4");

	REQUIRE(Hash::uint64(val) == 15148287618757152996);
}
}