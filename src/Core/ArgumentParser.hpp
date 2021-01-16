/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ARGUMENT_PARSER_HPP
#define CHALET_ARGUMENT_PARSER_HPP

#include "State/CommandLineInputs.hpp"

namespace chalet
{
class ArgumentParser
{
public:
	explicit ArgumentParser(CommandLineInputs& inInputs);

	bool run(const int argc, const char* const argv[]);

private:
	CommandLineInputs& m_inputs;
};
}

#endif // CHALET_ARGUMENT_PARSER_HPP
