/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct CommandLineInputs;

struct DotEnvFileParser
{
	explicit DotEnvFileParser(const CommandLineInputs& inInputs);

	bool readVariablesFromInputs();
	bool readVariablesFromFile(const std::string& inFile) const;

private:
	const CommandLineInputs& m_inputs;
};
}
