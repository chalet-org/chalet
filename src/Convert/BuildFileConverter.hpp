/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct CommandLineInputs;

struct BuildFileConverter
{
	explicit BuildFileConverter(const CommandLineInputs& inInputs);

	bool convertFromInputs();

private:
	bool convert(const std::string& inFormat, const std::string& inFile);

	const CommandLineInputs& m_inputs;
};
}
