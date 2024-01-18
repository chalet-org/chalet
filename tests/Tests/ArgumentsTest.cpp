#include "TestCase.hpp"

#include "Process/Process.hpp"
#include "Utility/String.hpp"

namespace chalet
{
TEST_CASE("chalet::ArgumentsTest", "[args]")
{
	const auto& exec = TestState::chaletExec();

	std::string res;
	res = Process::runOutput({ exec, "badcmd" });
	REQUIRE(String::endsWith("Invalid subcommand: 'badcmd'. See 'chalet --help'.", res));

	res = Process::runOutput({ exec, "configure", "extraarg" });
	REQUIRE(String::endsWith("Unknown argument: 'extraarg'. See 'chalet configure --help'.", res));

	res = Process::runOutput({ exec, "-z", "bogus" });
	REQUIRE(String::endsWith("Unknown argument: '-z'. See 'chalet --help'.", res));

	res = Process::runOutput({ exec, "set" });
	REQUIRE(String::endsWith("Missing required argument: '<key>'. See 'chalet set --help'.", res));
}
}