/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DOT_ENV_FILE_PARSER_HPP
#define CHALET_DOT_ENV_FILE_PARSER_HPP

namespace chalet
{
struct CommandLineInputs;

struct DotEnvFileParser
{
	explicit DotEnvFileParser(const CommandLineInputs& inInputs);

	bool readVariablesFromInputs();
	bool readVariablesFromFile(const std::string& inFile) const;

private:
	std::string searchDotEnv(const std::string& inRelativeEnv, const std::string& inEnv) const;

	const CommandLineInputs& m_inputs;
};
}

#endif // CHALET_DOT_ENV_FILE_PARSER_HPP
