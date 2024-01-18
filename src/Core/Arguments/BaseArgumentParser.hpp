/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BaseArgumentParser
{
	using RawArgumentList = std::map<std::string, std::string>;

public:
	BaseArgumentParser() = default;
	CHALET_DISALLOW_COPY_MOVE(BaseArgumentParser);
	virtual ~BaseArgumentParser() = default;

protected:
	StringList getArgumentList(const i32 argc, const char* argv[]);

	bool parse(StringList&& args, const u32 inPositionalArgs = 0);

	bool parseArgument(size_t& index, std::string& arg, const std::string& nextArg);
	void parseArgumentValue(std::string& arg);

	virtual StringList getTruthyArguments() const = 0;

	bool containsOption(const std::string& inOption);
	bool containsOption(const std::string& inShort, const std::string& inLong);

	RawArgumentList m_rawArguments;
	StringList m_remainingArguments;

private:
	StringList m_truthyArguments;
};
}
