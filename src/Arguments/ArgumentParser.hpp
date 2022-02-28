/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ARGUMENT_PARSER_HPP
#define CHALET_ARGUMENT_PARSER_HPP

namespace chalet
{
struct CommandLineInputs;

class ArgumentParser
{
public:
	explicit ArgumentParser(CommandLineInputs& inInputs);

	bool run(const int argc, const char* const argv[]);

private:
	StringList parseRawArguments(const int argc, const char* const argv[]);

	CommandLineInputs& m_inputs;
};
}

#endif // CHALET_ARGUMENT_PARSER_HPP
