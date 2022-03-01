/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CLI_PARSER_HPP
#define CHALET_CLI_PARSER_HPP

namespace chalet
{
namespace CLIParser
{
using RawArgumentList = std::vector<std::pair<std::string, std::string>>;

RawArgumentList parse(const int argc, const char* argv[], const int inPositionalArgs = 0);
std::optional<std::string> getOptionValue(const char** inBegin, const char** inEnd, const std::string& inOption);
bool optionExists(const char** inBegin, const char** inEnd, const std::string& inOption);
};
}

#endif // CHALET_CLI_PARSER_HPP
