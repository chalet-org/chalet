#include "TestCase.hpp"

#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
TEST_CASE("chalet::ArgumentsTest", "[args]")
{
	const auto& exec = TestState::chaletExec();

	std::string res;
	res = Commands::subprocessOutput({ exec, "badarg" });
	REQUIRE(String::endsWith("Invalid subcommand requested. See 'chalet --help'.", res));

	res = Commands::subprocessOutput({ exec, "configure", "extraarg" });
	REQUIRE(String::endsWith("Maximum number of positional arguments exceeded See 'chalet --help' or 'chalet <subcommand> --help'.", res));

	res = Commands::subprocessOutput({ exec, "-z", "bogus" });
	REQUIRE(String::endsWith("Unknown argument. See 'chalet --help' or 'chalet <subcommand> --help'.", res));

	res = Commands::subprocessOutput({ exec, "set" });
	REQUIRE(String::endsWith("1 argument(s) expected. 0 provided. See 'chalet --help' or 'chalet <subcommand> --help'.", res));
}
}