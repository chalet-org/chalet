/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ARGUMENT_READER_HPP
#define CHALET_ARGUMENT_READER_HPP

namespace chalet
{
struct CommandLineInputs;

class ArgumentReader
{
public:
	explicit ArgumentReader(CommandLineInputs& inInputs);

	bool run(const int argc, const char* argv[]);

private:
	CommandLineInputs& m_inputs;
};
}

#endif // CHALET_ARGUMENT_READER_HPP
