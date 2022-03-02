/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CLI_PARSER_HPP
#define CHALET_CLI_PARSER_HPP

namespace chalet
{
class CLIParser
{
	using RawArgumentList = std::unordered_map<std::string, std::string>;

public:
	CLIParser() = default;
	CHALET_DISALLOW_COPY_MOVE(CLIParser);
	virtual ~CLIParser() = default;

protected:
	bool parse(const int argc, const char* argv[], const int inPositionalArgs = 0);

	virtual StringList getTruthyArguments() const = 0;

	bool containsOption(const std::string& inOption);
	bool containsOption(const std::string& inShort, const std::string& inLong);

	RawArgumentList m_rawArguments;

private:
	std::optional<std::string> getOptionValue(const char** inBegin, const char** inEnd, const std::string& inOption);
	bool optionExists(const char** inBegin, const char** inEnd, const std::string& inOption);
};
}

#endif // CHALET_CLI_PARSER_HPP
