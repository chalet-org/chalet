#include "TestCase.hpp"

#include "Terminal/Files.hpp"
#include "Utility/String.hpp"

namespace chalet
{
TEST_CASE("chalet::ArgumentsTest", "[args]")
{
	const auto& exec = TestState::chaletExec();

	std::string res;
	res = Files::subprocessOutput({ exec, "badcmd" });
	REQUIRE(String::endsWith("Invalid subcommand: 'badcmd'. See 'chalet --help'.", res));

	res = Files::subprocessOutput({ exec, "configure", "extraarg" });
	REQUIRE(String::endsWith("Maximum number of positional arguments exceeded. See 'chalet configure --help'.", res));

	res = Files::subprocessOutput({ exec, "-z", "bogus" });
	REQUIRE(String::endsWith("Invalid argument(s) found. See 'chalet --help'.", res));

	res = Files::subprocessOutput({ exec, "set" });
	REQUIRE(String::endsWith("Missing required argument: '<key>'. See 'chalet set --help'.", res));
}
}